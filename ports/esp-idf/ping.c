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

#include "net/ping.h"

#include <semaphore.h>

#include "libmcu/metrics.h"
#include "ping/ping_sock.h"
#include "lwip/ip_addr.h"

struct cb_ctx {
	sem_t sem;
	uint32_t elapsed_time;
	int err;
};

static void on_ping_success(esp_ping_handle_t hdl, void *args)
{
	struct cb_ctx *ctx = (struct cb_ctx *)args;
	esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP,
			&ctx->elapsed_time, sizeof(ctx->elapsed_time));
	ctx->err = 0;
}

static void on_ping_timeout(esp_ping_handle_t hdl, void *args)
{
	unused(hdl);

	struct cb_ctx *ctx = (struct cb_ctx *)args;
	ctx->err = -ETIMEDOUT;
}

static void on_ping_end(esp_ping_handle_t hdl, void *args)
{
	struct cb_ctx *ctx = (struct cb_ctx *)args;
	esp_ping_delete_session(hdl);
	sem_post(&ctx->sem);
}

int ping_measure(const char *ipstr, const int timeout_ms)
{
	esp_ping_config_t conf = ESP_PING_DEFAULT_CONFIG();
	struct cb_ctx ctx = { 0, };

	sem_init(&ctx.sem, 0, 0);

	ipaddr_aton(ipstr, &conf.target_addr);
	conf.count = 1;
	conf.timeout_ms = (uint32_t)timeout_ms;

	esp_ping_callbacks_t cbs = {
		.cb_args = &ctx,
		.on_ping_success = on_ping_success,
		.on_ping_timeout = on_ping_timeout,
		.on_ping_end = on_ping_end,
	};

	esp_ping_handle_t ping;
	ESP_ERROR_CHECK(esp_ping_new_session(&conf, &cbs, &ping));
	esp_ping_start(ping);

	sem_wait(&ctx.sem);
	sem_destroy(&ctx.sem);

	if (ctx.err) {
		metrics_increase(PingFailureCount);
		return ctx.err;
	}

	metrics_set_max_min(PingTimeMax, PingTimeMin,
			METRICS_VALUE(ctx.elapsed_time));

	return (int)ctx.elapsed_time;
}
