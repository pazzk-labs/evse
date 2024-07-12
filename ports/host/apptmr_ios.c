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

#include "libmcu/apptmr.h"
#include <string.h>
#include <errno.h>
#include <dispatch/dispatch.h>

#if !defined(APPTMR_MAX)
#define APPTMR_MAX			8
#endif

#if !defined(APPTMR_INFO)
#define APPTMR_INFO(...)
#endif

struct apptmr {
	struct apptmr_api api;

	apptmr_callback_t callback;
	void *arg;
	dispatch_source_t handle;
	bool periodic;
};

static int count_created;

static struct apptmr *new_apptmr(struct apptmr *pool, size_t n)
{
	const struct apptmr empty = { 0, };

	for (size_t i = 0; i < n; i++) {
		if (memcmp(&empty, &pool[i], sizeof(empty)) == 0) {
			return &pool[i];
		}
	}

	return NULL;
}

static void free_apptmr(struct apptmr *apptmr)
{
	memset(apptmr, 0, sizeof(*apptmr));
}

static void callback_wrapper(void *ctx)
{
	struct apptmr *p = (struct apptmr *)ctx;

	apptmr_global_pre_timeout_hook(p);

	if (p && p->callback) {
		(*p->callback)(p, p->arg);
	}

	apptmr_global_post_timeout_hook(p);
}

static void trigger(struct apptmr *self)
{
	callback_wrapper(self);
}

static int start_apptmr(struct apptmr *self, const uint32_t timeout_ms)
{
	int err;
	uint64_t interval = self->periodic ?
		timeout_ms * NSEC_PER_MSEC : DISPATCH_TIME_FOREVER;

	dispatch_source_set_timer(self->handle, dispatch_time(DISPATCH_TIME_NOW,
			timeout_ms * NSEC_PER_MSEC), interval, 0);
	dispatch_source_set_event_handler_f(self->handle, callback_wrapper);
	dispatch_set_context(self->handle, self);
	dispatch_resume(self->handle);

	return 0;
}

static int restart_apptmr(struct apptmr *self, const uint32_t timeout_ms)
{
	return -ENOTSUP;
}

static int stop_apptmr(struct apptmr *self)
{
	dispatch_source_cancel(self->handle);
	dispatch_release(self->handle);
}

static int enable_apptmr(struct apptmr *self)
{
	return 0;
}

static int disable_apptmr(struct apptmr *self)
{
	return 0;
}

int apptmr_cap(void)
{
	return APPTMR_MAX;
}

int apptmr_len(void)
{
	return count_created;
}

struct apptmr *apptmr_create(bool periodic, apptmr_callback_t cb, void *cb_ctx)
{
	static struct apptmr apptmrs[APPTMR_MAX];
	struct apptmr *apptmr = new_apptmr(apptmrs, APPTMR_MAX);

	if (apptmr) {
		*apptmr = (struct apptmr) {
			.api = {
				.enable = enable_apptmr,
				.disable = disable_apptmr,
				.start = start_apptmr,
				.restart = restart_apptmr,
				.stop = stop_apptmr,
				.trigger = trigger,
			},

			.callback = cb,
			.arg = cb_ctx,
			.periodic = periodic,
		};

		dispatch_queue_t queue = dispatch_get_main_queue();
		apptmr->handle = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);

		count_created++;
		APPTMR_INFO("apptmr created(%d): %p\n", count_created, apptmr);
	}

	return apptmr;
}

int apptmr_delete(struct apptmr *self)
{
	dispatch_source_cancel(self->handle);
	dispatch_release(self->handle);

	free_apptmr(self);
	count_created--;
	APPTMR_INFO("apptmr deleted(%d): %p\n", count_created, self);
	return 0;
}
