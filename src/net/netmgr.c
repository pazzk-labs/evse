/*
 * This file is part of the Pazzk project <https://pazzk.net/>.
 * Copyright (c) 2024 Pazzk <team@pazzk.net>.
 *
 * Community Version License (GPLv3):
 * This software is open-source and licensed under the GNU General Public
 * License v3.0 (GPLv3). You are free to use, modify, and distribute this code
 * under the terms of the GPLv3. For more details, see
 * <https://www.gnu.org/licenses/gpl-3.0.en.html>.
 * Note: If you modify and distribute this software, you must make your
 * modifications publicly available under the same license (GPLv3), including
 * the source code.
 *
 * Commercial Version License:
 * For commercial use, including redistribution or integration into proprietary
 * systems, you must obtain a commercial license. This license includes
 * additional benefits such as dedicated support and feature customization.
 * Contact us for more details.
 *
 * Contact Information:
 * Maintainer: 권경환 Kyunghwan Kwon (on behalf of the Pazzk Team)
 * Email: k@pazzk.net
 * Website: <https://pazzk.net/>
 *
 * Disclaimer:
 * This software is provided "as-is", without any express or implied warranty,
 * including, but not limited to, the implied warranties of merchantability or
 * fitness for a particular purpose. In no event shall the authors or
 * maintainers be held liable for any damages, whether direct, indirect,
 * incidental, special, or consequential, arising from the use of this software.
 */

#include "net/netmgr.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>

#include "libmcu/list.h"
#include "libmcu/fsm.h"
#include "libmcu/apptmr.h"
#include "libmcu/metrics.h"
#include "libmcu/board.h"
#include "libmcu/wdt.h"
#include "libmcu/retry.h"
#include "libmcu/timext.h"
#include "libmcu/msgq.h"

#include "net/ntp.h"
#include "net/ping.h"
#include "app.h"
#include "logger.h"

#define STACK_SIZE_BYTES		5120U

#define DEFAULT_CONNECT_TIMEOUT_MS	NETMGR_CONNECT_TIMEOUT_MS
#define DEFAULT_PING_TIMEOUT_MS		100

#define DEFAULT_REENABLE_DELAY_MS	1000 /* netif re-enable delay */

#define DEFAULT_MAX_RETRY		NETMGR_MAX_RETRY
#define DEFAULT_MAX_BACKOFF_MS		NETMGR_MAX_BACKOFF_MS
#define DEFAULT_MIN_BACKOFF_MS		DEFAULT_CONNECT_TIMEOUT_MS
#define DEFAULT_MAX_JITTER_MS		DEFAULT_MIN_BACKOFF_MS

#define MASK_ALL_EVENTS			(0xFFu)

#define MAX_MESSAGES			8

enum {
	S0, /* Off */
	S1, /* Initializing */
	S2, /* Initialized or Disabled: Standby */
	S3, /* Enabling */
	S4, /* Enabled */
	S5, /* Connecting */
	S6, /* Connected */
	Sn, /* Number of states */
};

struct netmgr_entry {
	struct fsm fsm;
	struct list link;

	struct netif *netif;
	int priority;
	uint8_t mac_addr[NETIF_MAC_ADDR_LEN];
	ip_info_t ip_info;
	ip_info_t static_ip;
	netif_event_t event;

	struct retry retry;
	uint32_t conn_timeout_ms;
	uint32_t ts;
	uint32_t ts_state_changed;
	uint32_t ts_ping;

	netmgr_state_t external_state;
	bool restart;
	int error_count; /* used by states between S2 and S6 */
};

struct netmgr_event_cb {
	struct list link;
	netmgr_state_mask_t mask;
	netmgr_event_cb_t cb;
	void *ctx;
};

struct netmgr_task_entry {
	struct list link;
	netmgr_task_t func;
	void *ctx;
};

struct netmgr {
	struct list netif_list;
	struct list conn_event_cb_list;
	struct list task_list;
	struct netmgr_entry *current;

	pthread_t thread;
	sem_t event;
	struct wdt *wdt;
	struct apptmr *timer;
	struct msgq *msgq;

	time_t time_ntp_synced;
	uint32_t healthchk_interval_ms;

	bool enabled;
	bool selftest_ping_requested;
	bool terminated;
};

static struct netmgr m;

static const char *stringify_state(const fsm_state_t state)
{
	switch (state) {
	case S0: return "S0";
	case S1: return "S1";
	case S2: return "S2";
	case S3: return "S3";
	case S4: return "S4";
	case S5: return "S5";
	case S6: return "S6";
	default: return "UNKNOWN";
	}
}

static netmgr_state_t get_event_from_state(struct netmgr_entry *entry,
		const fsm_state_t next, const fsm_state_t prev)
{
	const netmgr_state_t states[Sn] = {
		[S0] = NETMGR_STATE_OFF,
		[S1] = NETMGR_STATE_OFF,
		[S2] = NETMGR_STATE_INITIALIZED,
		[S3] = NETMGR_STATE_INITIALIZED,
		[S4] = NETMGR_STATE_ENABLED,
		[S5] = NETMGR_STATE_ENABLED,
		[S6] = NETMGR_STATE_CONNECTED,
	};
	netmgr_state_t state = states[next];

	if (next == S0 && retry_exhausted(&entry->retry)) {
		state = NETMGR_STATE_EXHAUSTED;
	} else if (next == S2 && prev > S2) {
		state = NETMGR_STATE_DISABLED;
	} else if (next == S4 && prev > S4) {
		state = NETMGR_STATE_DISCONNECTED;
	}

	return state;
}

static bool check_netmgr_enabled(void)
{
	return m.enabled;
}

static bool check_task_registered(void)
{
	return !list_empty(&m.task_list);
}

static bool check_selftest_ping_requested(void)
{
	return m.selftest_ping_requested;
}

static void request_selftest_ping(void)
{
	m.selftest_ping_requested = true;
}

static void clear_selftest_ping(void)
{
	m.selftest_ping_requested = false;
}

static void clear_connection_event_cb(void)
{
	struct list *p, *n;
	list_for_each_safe(p, n, &m.conn_event_cb_list) {
		struct netmgr_event_cb *entry =
			list_entry(p, struct netmgr_event_cb, link);
		list_del(p, &m.conn_event_cb_list);
		free(entry);
	}
}

static void clear_netif_list(void)
{
	struct list *p, *n;
	list_for_each_safe(p, n, &m.netif_list) {
		struct netmgr_entry *entry =
			list_entry(p, struct netmgr_entry, link);
		list_del(p, &m.netif_list);
		free(entry);
	}
}

static void add_netif(struct netmgr_entry *entry)
{
	struct list **p = &m.netif_list.next;
	while (*p != &m.netif_list) {
		const struct netmgr_entry *t =
			list_entry(*p, struct netmgr_entry, link);
		if (entry->priority > t->priority) {
			break;
		}
		p = &(*p)->next;
	}
	entry->link.next = *p;
	*p = &entry->link;
}

static void dispatch_event(const netmgr_state_t event)
{
	struct list *p;
	list_for_each(p, &m.conn_event_cb_list) {
		struct netmgr_event_cb *cb =
			list_entry(p, struct netmgr_event_cb, link);
		if (cb->cb && (cb->mask & event)) {
			(*cb->cb)(event, cb->ctx);
		}
	}
}

static void start_ping_timer(void)
{
	apptmr_start(m.timer, m.healthchk_interval_ms? m.healthchk_interval_ms
			: DEFAULT_CONNECT_TIMEOUT_MS / 2);
}

static void stop_ping_timer(void)
{
	apptmr_stop(m.timer);
}

static struct netmgr_entry *pick_netif(struct list *list,
		struct netmgr_entry *current)
{
	struct netmgr_entry *next = NULL;

	if (!current) {
		next = list_entry(list->next, struct netmgr_entry, link);
		next = &next->link == list ? NULL : next;
	} else {
		/* TODO: implement some algorithm to pick the next entry */
		next = current;
	}

	return next;
}

static void on_state_change(struct fsm *fsm,
		fsm_state_t new_state, fsm_state_t prev_state, void *ctx)
{
	unused(fsm);

	struct netmgr_entry *entry = (struct netmgr_entry *)ctx;
	const netmgr_state_t event =
		get_event_from_state(entry, new_state, prev_state);

	entry->ts_state_changed = entry->ts;

	info("state changed from %s to %s", stringify_state(prev_state),
			stringify_state(new_state));

	if (event != entry->external_state) {
		entry->external_state = event;
		if (new_state < prev_state) {
			if (prev_state >= S6) { /* disconnected */
				dispatch_event(NETMGR_STATE_DISCONNECTED);
			}
			if (prev_state >= S4) { /* disabled */
				dispatch_event(NETMGR_STATE_DISABLED);
			}
		}
		dispatch_event(event);
	}
}

static void on_netif_event(struct netif *netif,
		const netif_event_t event, void *ctx)
{
	unused(netif);
	unused(ctx);

	msgq_push(m.msgq, &event, sizeof(event));
	sem_post(&m.event);
	metrics_set(SystemEventStackHighWatermark,
			METRICS_VALUE(board_get_current_stack_watermark()));
}

static void on_periodic_timer(struct apptmr *timer, void *arg)
{
	unused(timer);

	struct netmgr *netmgr = (struct netmgr *)arg;

	if (netmgr->healthchk_interval_ms) {
		request_selftest_ping();
	}

	sem_post(&netmgr->event);
	metrics_increase(NetMgrPingTimerCount);
}

static void on_network_time_sync(struct timeval *tv, void *ctx)
{
	struct netmgr *netmgr = (struct netmgr *)ctx;
	netmgr->time_ntp_synced = time(NULL);
	metrics_increase(NTPSyncCount);
	info("ntp sync: %lu", tv->tv_sec);
}

static bool is_static_ip_requested(struct netmgr_entry *entry)
{
	ip_info_t empty = { 0, };
	return memcmp(&entry->static_ip, &empty, sizeof(empty)) != 0;
}

static bool is_error(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);
	struct netmgr_entry *entry = (struct netmgr_entry *)ctx;
	return check_netmgr_enabled() && (retry_exhausted(&entry->retry) ||
		entry->error_count >= (int)DEFAULT_MAX_RETRY);
}

static bool is_state_s0(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(next_state);
	struct netmgr_entry *entry = (struct netmgr_entry *)ctx;
	const uint32_t elapsed = entry->ts - entry->ts_state_changed;
	const bool timeout = elapsed >= entry->conn_timeout_ms;

	if (!check_netmgr_enabled() || /* turned off */
			entry->restart || /* error occurred */
			retry_exhausted(&entry->retry) || /* too many retries */
			(state == S1 && timeout)) { /* timeout */
		return true;
	}

	return false;
}

static bool is_state_s1(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	return check_netmgr_enabled() && !is_error(state, next_state, ctx);
}

static bool is_state_s2(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(next_state);
	struct netmgr_entry *entry = (struct netmgr_entry *)ctx;
	const uint32_t elapsed = entry->ts - entry->ts_state_changed;
	const bool timeout = elapsed >= entry->conn_timeout_ms;

	if (state == S1) {
		return true;
	} else if (state == S3 && timeout) {
		return true;
	}

	return entry->error_count;
}

static bool is_state_s3(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);
	struct netmgr_entry *entry = (struct netmgr_entry *)ctx;
	const uint32_t elapsed = entry->ts - entry->ts_state_changed;
	return elapsed >= DEFAULT_REENABLE_DELAY_MS;
}

static bool is_state_s4(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(next_state);
	struct netmgr_entry *entry = (struct netmgr_entry *)ctx;
	const uint32_t elapsed = entry->ts - entry->ts_state_changed;
	const bool timeout = elapsed >= entry->conn_timeout_ms;

	if (state == S3) {
		return true;
	} else if (state == S5) {
		return timeout || entry->event == NETIF_EVENT_DISCONNECTED;
	} else if (state == S6) {
		return entry->event != NETIF_EVENT_IP_ACQUIRED;
	}

	return false;
}

static bool is_state_s5(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);
	struct netmgr_entry *entry = (struct netmgr_entry *)ctx;
	const uint32_t elapsed = entry->ts - entry->ts_state_changed;

	return retry_first(&entry->retry) ||
		elapsed >= retry_get_backoff(&entry->retry);
}

static bool is_state_s6(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);
	struct netmgr_entry *entry = (struct netmgr_entry *)ctx;
	return entry->event == NETIF_EVENT_IP_ACQUIRED;
}

static bool is_ping_req(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);
	unused(ctx);
	return check_selftest_ping_requested();
}

static bool is_task_req(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);
	unused(ctx);
	return check_task_registered();
}

static void do_ping(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);

	struct netmgr_entry *entry = (struct netmgr_entry *)ctx;
	char ip_str[16];

	clear_selftest_ping();

	snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d",
			entry->ip_info.v4.gateway.addr[0],
			entry->ip_info.v4.gateway.addr[1],
			entry->ip_info.v4.gateway.addr[2],
			entry->ip_info.v4.gateway.addr[3]);
	const int elapsed_time = ping_measure(ip_str, DEFAULT_PING_TIMEOUT_MS);

	if (elapsed_time < 0) {
		entry->error_count++;
		metrics_increase(NetMgrPingFailureCount);
		error("ping failed: %d", elapsed_time);
	} else {
		const uint32_t now = entry->ts;

		if (entry->ts_ping) {
			const uint32_t interval = now - entry->ts_ping;

			metrics_set_if_max(NetMgrPingIntervalMax,
					METRICS_VALUE(interval));
			metrics_set_if_min(NetMgrPingIntervalMin,
					METRICS_VALUE(interval));
		}

		entry->ts_ping = now;
		debug("ping to %s: %dms", ip_str, elapsed_time);
	}
}

static void do_task(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);
	unused(ctx);

	bool repeat = false;

	struct list *p;
	struct list *n;
	list_for_each_safe(p, n, &m.task_list) {
		struct netmgr_task_entry *t =
			list_entry(p, struct netmgr_task_entry, link);
		if (!(*t->func)(t->ctx)) { /* one-time task */
			list_del(p, &m.task_list);
			free(t);
		} else {
			repeat = true;
		}
	}

	if (repeat) {
		sem_post(&m.event);
	}
}

static void do_connected(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);
	struct netmgr_entry *entry = (struct netmgr_entry *)ctx;

	retry_reset(&entry->retry);
	ntp_start(on_network_time_sync, &m);
	start_ping_timer();

	metrics_increase(NetMgrConnectedCount);
}

static void do_disconnect(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(next_state);
	struct netmgr_entry *entry = (struct netmgr_entry *)ctx;
	struct netif *netif = entry->netif;
	struct netif_api *api = (struct netif_api *)netif;

	int err = api->disconnect(netif);
	metrics_increase(NetMgrDisconnectCount);

	if (state == S6) {
		stop_ping_timer();
	}

	if (err) {
		entry->error_count++;
		error("netif disconnect failed: %d", err);
		return;
	}
}

static void do_connect(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);
	struct netmgr_entry *entry = (struct netmgr_entry *)ctx;
	struct netif *netif = entry->netif;
	struct netif_api *api = (struct netif_api *)netif;
	uint32_t backoff_time;

	retry_backoff(&entry->retry, &backoff_time, (uint16_t)board_random());

	int err = api->connect(netif);
	metrics_increase(NetMgrConnectCount);

	if (err) {
		entry->error_count++;
		metrics_increase(NetMgrNetifConnectFailureCount);
		error("netif connect failed: %d", err);
		return;
	}
}

static void do_enabled(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);
	unused(ctx);
}

static void do_disable(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct netmgr_entry *entry = (struct netmgr_entry *)ctx;
	struct netif *netif = entry->netif;
	struct netif_api *api = (struct netif_api *)netif;

	if (state >= S5) {
		do_disconnect(state, next_state, ctx);
	}

	int err = api->disable(netif);
	metrics_increase(NetMgrDisableCount);

	if (err) {
		error("netif disable failed: %d", err);
		return;
	}
}

static void do_enable(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);
	struct netmgr_entry *entry = (struct netmgr_entry *)ctx;
	struct netif *netif = entry->netif;
	struct netif_api *api = (struct netif_api *)netif;

	int err = api->enable(netif);
	metrics_increase(NetMgrEnableCount);

	if (err) {
		entry->restart = true;
		entry->error_count++;
		metrics_increase(NetMgrNetifEnableFailureCount);
		error("netif enable failed: %d", err);
		return;
	}

	entry->error_count = 0;
}

static void do_initialized(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);
	unused(ctx);
}

static void do_off(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct netmgr_entry *entry = (struct netmgr_entry *)ctx;
	struct netif *netif = entry->netif;
	struct netif_api *api = (struct netif_api *)netif;

	if (state >= S3) {
		do_disable(state, next_state, ctx);
	}

	int err = api->unregister_event_callback(netif,
			NETIF_EVENT_ANY, on_netif_event);
	err |= api->deinit(netif);

	if (err) {
		error("netif deinit failed: %d", err);
	}

	wdt_disable(m.wdt);
	entry->restart = false;
}

static void do_on(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);
	struct netmgr_entry *entry = (struct netmgr_entry *)ctx;
	struct netif *netif = entry->netif;
	struct netif_api *api = (struct netif_api *)netif;

	int err = api->init(netif, entry->mac_addr);
	err |= api->register_event_callback(netif, NETIF_EVENT_ANY,
			on_netif_event, entry);

	if (is_static_ip_requested(entry)) {
		err |= api->set_ip_info(netif, &entry->static_ip);
	}

	if (err) {
		entry->restart = true;
		entry->error_count++;
		error("netif init failed: %d", err);
		metrics_increase(NetMgrNetifInitFailureCount);
		return;
	}

	wdt_enable(m.wdt);
	metrics_increase(NetMgrBootCount);
}

static void do_reset(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);
	unused(ctx);
	error("unrecoverable error state. rebooting...");
	app_reboot();
}

static const struct fsm_item transitions[] = {
	FSM_ITEM(S0, is_state_s1, do_on,          S1),
	FSM_ITEM(S0, is_error,    do_reset,       S0),
	/* Initializing */
	FSM_ITEM(S1, is_state_s0, do_off,         S0),
	FSM_ITEM(S1, is_state_s2, do_initialized, S2),
	/* Initialized or Disabled */
	FSM_ITEM(S2, is_state_s0, do_off,         S0),
	FSM_ITEM(S2, is_state_s3, do_enable,      S3),
	/* Enabling */
	FSM_ITEM(S3, is_state_s0, do_off,         S0),
	FSM_ITEM(S3, is_state_s2, do_disable,     S2),
	FSM_ITEM(S3, is_state_s4, do_enabled,     S4),
	/* Enabled */
	FSM_ITEM(S4, is_state_s0, do_off,         S0),
	FSM_ITEM(S4, is_state_s2, do_disable,     S2),
	FSM_ITEM(S4, is_state_s5, do_connect,     S5),
	/* Connecting */
	FSM_ITEM(S5, is_state_s0, do_off,         S0),
	FSM_ITEM(S5, is_state_s2, do_disable,     S2),
	FSM_ITEM(S5, is_state_s4, do_disconnect,  S4),
	FSM_ITEM(S5, is_state_s6, do_connected,   S6),
	/* Connected */
	FSM_ITEM(S6, is_state_s0, do_off,         S0),
	FSM_ITEM(S6, is_state_s2, do_disable,     S2),
	FSM_ITEM(S6, is_state_s4, do_disconnect,  S4),
	FSM_ITEM(S6, is_ping_req, do_ping,        S6),
	FSM_ITEM(S6, is_task_req, do_task,        S6),
};

/* The event of IP acquired must be arrived after the event of connected.
 * Any other order or events are considered as an error. */
static void process_netif_event(struct netmgr_entry *entry,
		const netif_event_t event)
{
	const struct netif_api *api = (const struct netif_api *)entry->netif;
	ip_info_t *ip_info = &entry->ip_info;
	uint32_t speed_kbps;
	bool duplex_enabled;

	switch (event) {
	case NETIF_EVENT_STARTED:
		info("netif up");
		break;
	case NETIF_EVENT_STOPPED:
		info("netif down");
		break;
	case NETIF_EVENT_CONNECTED:
		api->get_speed(entry->netif, &speed_kbps);
		api->get_duplex(entry->netif, &duplex_enabled);
		info("netif connected: %u kbps, %s duplex",
				speed_kbps, duplex_enabled ? "full" : "half");
		break;
	case NETIF_EVENT_DISCONNECTED:
		info("netif disconnected");
		break;
	case NETIF_EVENT_IP_ACQUIRED:
		api->get_ip_info(entry->netif, ip_info);
		info("ip %d.%d.%d.%d, netmask %d.%d.%d.%d, gateway %d.%d.%d.%d",
				ip_info->v4.ip.addr[0], ip_info->v4.ip.addr[1],
				ip_info->v4.ip.addr[2], ip_info->v4.ip.addr[3],
				ip_info->v4.netmask.addr[0],
				ip_info->v4.netmask.addr[1],
				ip_info->v4.netmask.addr[2],
				ip_info->v4.netmask.addr[3],
				ip_info->v4.gateway.addr[0],
				ip_info->v4.gateway.addr[1],
				ip_info->v4.gateway.addr[2],
				ip_info->v4.gateway.addr[3]);
		break;
	default:
		entry->restart = true;
		warn("unknown event: %d", event);
		break;
	}

	entry->event = event;
}

static void process_event(struct netmgr *netmgr)
{
	netif_event_t event;

	while (netmgr->current &&
			msgq_pop(netmgr->msgq, &event, sizeof(event)) > 0) {
		process_netif_event(netmgr->current, event);
	}
}

static void process(struct netmgr *netmgr)
{
	netmgr->current = pick_netif(&netmgr->netif_list, netmgr->current);

	if (!netmgr->current) { /* no network interface found */
		return;
	}

	netmgr->current->ts = board_get_time_since_boot_ms();
	const fsm_state_t state = fsm_step(&netmgr->current->fsm);

	if (state == S0 && retry_exhausted(&netmgr->current->retry)) {
		/* disabled or unavailable */
		return;
	} else if (state != S6) {
		/* continues to connect until it reaches to S4 or exhausted */
		sem_post(&netmgr->event);
		sleep_ms(10); /* without this, the task will be busy */
	}
}

static void shutdown_netmgr(void)
{
	/* cleanup */
	sem_destroy(&m.event);
	apptmr_delete(m.timer);
	wdt_delete(m.wdt);
	msgq_destroy(m.msgq);

	clear_connection_event_cb();
	clear_netif_list();
}

static void *thread(void *e)
{
	struct netmgr *netmgr = (struct netmgr *)e;

	while (!netmgr->terminated) {
		sem_wait(&netmgr->event);
		wdt_feed(netmgr->wdt);

		process_event(netmgr);
		process(netmgr);

		metrics_set(NetMgrStackHighWatermark,
			METRICS_VALUE(board_get_current_stack_watermark()));
	}

	shutdown_netmgr();

	return 0;
}

bool netmgr_connected(void)
{
	return m.current && m.current->event == NETIF_EVENT_IP_ACQUIRED;
}

netmgr_state_t netmgr_state(void)
{
	if (!m.current) {
		return NETMGR_STATE_OFF;
	}
	return m.current->external_state;
}

int netmgr_ping(const char *ipstr, const uint32_t timeout_ms)
{
	unused(timeout_ms);
	if (!m.current) {
		return -ENODEV;
	} else if (m.current->external_state != NETMGR_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	return ping_measure(ipstr, DEFAULT_PING_TIMEOUT_MS);
}

int netmgr_register_task(netmgr_task_t task_func, void *task_ctx)
{
	struct netmgr_task_entry *entry =
		(struct netmgr_task_entry *)malloc(sizeof(*entry));

	if (!entry) {
		return -ENOMEM;
	}

	*entry = (struct netmgr_task_entry) {
		.func = task_func,
		.ctx = task_ctx,
	};

	list_add_tail(&entry->link, &m.task_list);

	sem_post(&m.event);

	return 0;
}

int netmgr_register_event_cb(const netmgr_state_mask_t mask,
		netmgr_event_cb_t cb, void *ctx)
{
	struct netmgr_event_cb *entry =
		(struct netmgr_event_cb *)malloc(sizeof(*entry));

	if (!entry) {
		return -ENOMEM;
	}

	*entry = (struct netmgr_event_cb) {
		.cb = cb,
		.ctx = ctx,
		.mask = !mask? MASK_ALL_EVENTS : mask,
	};

	list_add(&entry->link, &m.conn_event_cb_list);

	return 0;
}

int netmgr_register_iface(struct netif *netif, const int priority,
		const uint8_t mac[NETIF_MAC_ADDR_LEN],
		const ip_info_t *static_ip)
{
	struct netmgr_entry *entry =
		(struct netmgr_entry *)calloc(1, sizeof(*entry));

	if (!entry) {
		return -ENOMEM;
	}

	*entry = (struct netmgr_entry) {
		.netif = netif,
		.priority = priority,
		.conn_timeout_ms = DEFAULT_CONNECT_TIMEOUT_MS,
		.external_state = NETMGR_STATE_OFF,
	};
	memcpy(entry->mac_addr, mac, sizeof(entry->mac_addr));

	if (static_ip) {
		entry->static_ip = *static_ip;
	}

	fsm_init(&entry->fsm, transitions,
			sizeof(transitions) / sizeof(*transitions), entry);
	fsm_set_state_change_cb(&entry->fsm, on_state_change, entry);
	retry_new_static(&entry->retry, &(struct retry_param) {
		.max_attempts = DEFAULT_MAX_RETRY,
		.min_backoff_ms = DEFAULT_MIN_BACKOFF_MS,
		.max_backoff_ms = DEFAULT_MAX_BACKOFF_MS,
		.max_jitter_ms = DEFAULT_MAX_JITTER_MS,
	});

	add_netif(entry);

	return 0;
}

int netmgr_enable(void)
{
	if (m.current) {
		retry_reset(&m.current->retry);
		m.current->error_count = 0;
	}

	m.enabled = true;
	sem_post(&m.event);

	apptmr_enable(m.timer);

	return 0;
}

int netmgr_disable(void)
{
	apptmr_disable(m.timer);
	m.enabled = false;
	sem_post(&m.event);

	return 0;
}

int netmgr_init(uint32_t healthchk_interval_ms)
{
	memset(&m, 0, sizeof(m));

	list_init(&m.netif_list);
	list_init(&m.conn_event_cb_list);
	list_init(&m.task_list);
	sem_init(&m.event, 0, 0);

	m.healthchk_interval_ms = healthchk_interval_ms;

	if ((m.wdt = wdt_new("net",
			healthchk_interval_ms + DEFAULT_CONNECT_TIMEOUT_MS,
			0, 0)) == NULL) {
		return -ENOMEM;
	}
	if ((m.timer = apptmr_create(true, on_periodic_timer, &m)) == NULL) {
		wdt_delete(m.wdt);
		return -ENOMEM;
	}
	if ((m.msgq = msgq_create(msgq_calc_size(MAX_MESSAGES,
			sizeof(netif_event_t)))) == NULL) {
		apptmr_delete(m.timer);
		wdt_delete(m.wdt);
		return -ENOMEM;
	}

#if defined(UNIT_TEST)
	return 0;
#endif
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, STACK_SIZE_BYTES);

	return pthread_create(&m.thread, &attr, thread, &m);
}

void netmgr_deinit(void)
{
	m.terminated = true;
	sem_post(&m.event);
#if defined(UNIT_TEST)
	shutdown_netmgr();
#endif
}

#if defined(UNIT_TEST)
int netmgr_step(void)
{
	process_event(&m);
	process(&m);
	return 0;
}
#endif
