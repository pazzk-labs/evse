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

#include "libmcu/wdt.h"

#include <pthread.h>
#include <stdlib.h>

#include "libmcu/list.h"
#include "libmcu/board.h"
#include "libmcu/timext.h"

#if !defined(MIN)
#define MIN(a, b)		((a) > (b)? (b) : (a))
#endif

#if !defined(WDT_ERROR)
#define WDT_ERROR(...)
#endif

struct wdt {
	const char *name;
	uint32_t period_ms; /* timeout period in milliseconds */
	uint32_t last_feed_ms; /* last feed time in milliseconds */

	wdt_timeout_cb_t cb;
	void *cb_ctx;

	struct list link;

	bool enabled;
};

struct wdt_manager {
	pthread_t thread;

	struct list list;

	wdt_timeout_cb_t cb;
	void *cb_ctx;

	wdt_periodic_cb_t periodic_cb;
	void *periodic_cb_ctx;

	uint32_t min_period_ms;
};

static struct wdt_manager m;

static bool is_timedout(struct wdt *wdt, const uint32_t now)
{
	return wdt->enabled && (now - wdt->last_feed_ms) >= wdt->period_ms;
}

static struct wdt *any_timeouts(struct wdt_manager *mgr, const uint32_t now)
{
	struct list *p;

	list_for_each(p, &mgr->list) {
		struct wdt *wdt = list_entry(p, struct wdt, link);
		if (is_timedout(wdt, now)) {
			return wdt;
		}
	}

	return NULL;
}

static void *wdt_task(void *e)
{
	struct wdt_manager *mgr = (struct wdt_manager *)e;

	while (1) {
		if (mgr->periodic_cb) {
			(*mgr->periodic_cb)(mgr->periodic_cb_ctx);
		}

		struct wdt *wdt = any_timeouts(mgr,
				board_get_time_since_boot_ms());

		if (wdt) {
			WDT_ERROR("wdt %s timed out", wdt->name);

			if (wdt->cb) {
				(*wdt->cb)(wdt, wdt->cb_ctx);
			}
			if (mgr->cb) {
				(*mgr->cb)(wdt, mgr->cb_ctx);
			}
		}

		sleep_ms(mgr->min_period_ms);
	}

	return 0;
}

int wdt_feed(struct wdt *self)
{
	self->last_feed_ms = board_get_time_since_boot_ms();
	return 0;
}

int wdt_enable(struct wdt *self)
{
	self->enabled = true;
	return 0;
}

int wdt_disable(struct wdt *self)
{
	self->enabled = false;
	return 0;
}

struct wdt *wdt_new(const char *name, const uint32_t period_ms,
		wdt_timeout_cb_t cb, void *cb_ctx)
{
	struct wdt *wdt = (struct wdt *)malloc(sizeof(*wdt));

	if (wdt) {
		wdt->name = name;
		wdt->period_ms = period_ms;
		wdt->cb = cb;
		wdt->cb_ctx = cb_ctx;
		list_add(&wdt->link, &m.list);

		m.min_period_ms = MIN(m.min_period_ms, period_ms);
	}
	
	return wdt;
}

void wdt_delete(struct wdt *self)
{
	list_del(&self->link, &m.list);
	free(self);
}

int wdt_register_timeout_cb(wdt_timeout_cb_t cb, void *cb_ctx)
{
	m.cb = cb;
	m.cb_ctx = cb_ctx;

	return 0;
}

const char *wdt_name(const struct wdt *self)
{
	return self->name;
}

int wdt_init(wdt_periodic_cb_t cb, void *cb_ctx)
{
	m.min_period_ms = 1000;
	m.periodic_cb = cb;
	m.periodic_cb_ctx = cb_ctx;
	list_init(&m.list);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	return pthread_create(&m.thread, &attr, wdt_task, &m);
}

void wdt_deinit(void)
{
	pthread_exit(&m.thread);
}
