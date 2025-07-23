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

#include "net/wifi.h"

#include <errno.h>
#include <string.h>

#include "libmcu/wifi.h"
#include "libmcu/board.h"
#include "libmcu/metrics.h"

#include "net/wifi_ap_info.h"

struct event_callback {
	netif_event_callback_t cb;
	void *cb_ctx;
	netif_event_t event_type;
};

struct ap_scan {
	uint32_t timestamp;
};

struct netif {
	struct netif_api api;
	const char *name;

	struct wifi *iface;

	wifi_connection_param_getter getter;
	void *getter_ctx;

	struct event_callback event_callback;
	struct ap_scan scan;

	uint8_t mac_addr[NETIF_MAC_ADDR_LEN];
	ip_info_t static_ip;
	ip_info_t ip_info;
};

static void dispatch_event(struct netif *netif, netif_event_t event_type)
{
	struct event_callback *p = &netif->event_callback;
	if (p->cb && (p->event_type == event_type ||
			p->event_type == NETIF_EVENT_ANY)) {
		(*p->cb)(netif, event_type, p->cb_ctx);
	}
}

static void on_wifi_events(const struct wifi *iface, enum wifi_event evt,
		const void *data, void *user_ctx)
{
	unused(iface);

	struct netif *netif = (struct netif *)user_ctx;
	netif_event_t netif_event = NETIF_EVENT_UNKNOWN;

	switch (evt) {
	case WIFI_EVT_SCAN_RESULT:
		break;
	case WIFI_EVT_SCAN_DONE:
		metrics_set_max_min(WiFiScanTimeMax, WiFiScanTimeMin,
				METRICS_VALUE(board_get_time_since_boot_ms() -
						netif->scan.timestamp));
		break;
	case WIFI_EVT_DISCONNECTED:
		netif_event = NETIF_EVENT_DISCONNECTED;
		metrics_increase(WiFiDisconnectCount);
		break;
	case WIFI_EVT_STARTED: /* enabled */
		netif_event = NETIF_EVENT_STARTED;
		break;
	case WIFI_EVT_CONNECTED:
		netif_event = NETIF_EVENT_CONNECTED;
		metrics_increase(WiFiConnectCount);
		break;
	case WIFI_EVT_STOPPED: /* disabled */
		netif_event = NETIF_EVENT_STOPPED;
		break;
	default:
		warn("unknown WIFI event: %x", evt);
		return;
	}

	dispatch_event(netif, netif_event);
	if (netif_event == NETIF_EVENT_CONNECTED) {
		dispatch_event(netif, NETIF_EVENT_IP_ACQUIRED);
	}
}

static int init(struct netif *netif, uint8_t mac_addr[NETIF_MAC_ADDR_LEN])
{
	memcpy(netif->mac_addr, mac_addr, NETIF_MAC_ADDR_LEN);
	return 0;
}

static int deinit(struct netif *netif)
{
	unused(netif);
	return 0;
}

static int enable(struct netif *netif)
{
	int err;

	if ((err = wifi_register_event_callback(netif->iface,
			WIFI_EVT_ANY, on_wifi_events, netif)) != 0) {
		return err;
	}

	return wifi_enable(netif->iface);
}

static int disable(struct netif *netif)
{
	return wifi_disable(netif->iface);
}

static int connect(struct netif *netif)
{
	struct wifi_ap_info ap_info;

	if (!netif->getter) {
		return -ENOENT;
	}
	if ((*netif->getter)(netif, &ap_info, netif->getter_ctx) != 0) {
		return -EINVAL;
	}

	struct wifi_conn_param param = {
		.ssid = (const uint8_t *)ap_info.ssid,
		.ssid_len = (uint8_t)strlen(ap_info.ssid),
		.psk = (const uint8_t *)ap_info.pass,
		.psk_len = (uint8_t)strlen(ap_info.pass),
		.security = ap_info.security,
	};

	info("Connecting to %s", param.ssid);

	return wifi_connect(netif->iface, &param);
}

static int disconnect(struct netif *netif)
{
	return wifi_disconnect(netif->iface);
}

static int register_event_callback(struct netif *netif,
		const netif_event_t event_type,
		const netif_event_callback_t cb, void *cb_ctx)
{
	netif->event_callback.cb = cb;
	netif->event_callback.cb_ctx = cb_ctx;
	netif->event_callback.event_type = event_type;

	return 0;
}

static int unregister_event_callback(struct netif *netif,
		const netif_event_t event_type, const netif_event_callback_t cb)
{
	memset(&netif->event_callback, 0, sizeof(netif->event_callback));
	return 0;
}

static int set_ip_info(struct netif *netif, const ip_info_t *ip_info)
{
	unused(netif);
	unused(ip_info);
	return -ENOTSUP;
}

static int get_ip_info(struct netif *netif, ip_info_t *ip_info)
{
	struct wifi_iface_info info;
	int err = wifi_get_status(netif->iface, &info);
	if (!err) {
		memcpy(ip_info, &info.ip_info, sizeof(*ip_info));
	}
	return 0;
}

static int get_speed(struct netif *netif, uint32_t *kbps)
{
	unused(netif);
	unused(kbps);
	return -ENOTSUP;
}

static int get_duplex(struct netif *netif, bool *duplex_enabled)
{
	unused(netif);
	unused(duplex_enabled);
	return -ENOTSUP;
}

static int send_eth_frame(struct netif *netif, const void *data,
			const size_t datasize)
{
	unused(netif);
	unused(data);
	unused(datasize);
	return -ENOTSUP;
}

struct netif *wifi_netif_create(struct wifi *iface,
		wifi_connection_param_getter getter, void *getter_ctx)
{
	static struct netif netif = {
		.api = {
			.init = init,
			.deinit = deinit,
			.enable = enable,
			.disable = disable,
			.connect = connect,
			.disconnect = disconnect,
			.register_event_callback = register_event_callback,
			.unregister_event_callback = unregister_event_callback,
			.set_ip_info = set_ip_info,
			.get_ip_info = get_ip_info,
			.get_speed = get_speed,
			.get_duplex = get_duplex,
			.send_eth_frame = send_eth_frame,
		},
		.name = "wifi",
	};

	netif.iface = iface;
	netif.getter = getter;
	netif.getter_ctx = getter_ctx;

	return &netif;
}
