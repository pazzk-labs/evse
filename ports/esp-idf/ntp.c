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

#include "net/ntp.h"
#include "esp_sntp.h"

static void (*on_time_sync_cb)(struct timeval *tv, void *ctx);
static void *on_time_sync_cb_ctx;
static bool initialized;

static void on_time_sync(struct timeval *tv)
{
	if (on_time_sync_cb) {
		(*on_time_sync_cb)(tv, on_time_sync_cb_ctx);
	}
}

static void init_and_start(void (*cb)(struct timeval *, void *ctx), void *ctx)
{
	on_time_sync_cb = cb;
	on_time_sync_cb_ctx = ctx;

	esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
	esp_sntp_setservername(0, "pool.ntp.org");
	sntp_set_time_sync_notification_cb(on_time_sync);
	esp_sntp_init();

	initialized = true;
}

int ntp_start(void (*cb)(struct timeval *, void *ctx), void *cb_ctx)
{
	if (!initialized) {
		init_and_start(cb, cb_ctx);
		return 0;
	}

	return -EALREADY;
}
