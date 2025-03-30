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

#include "downloader_http.h"

#include <string.h>
#include <errno.h>

#include "net/http_client.h"
#include "logger.h"

struct downloader {
	struct downloader_api api;
	struct downloader_param param;

	downloader_data_cb_t data_cb;
	void *data_cb_ctx;

	struct http_client *handle;
};

static void on_data_reception(const void *data, size_t datasize, void *ctx)
{
	struct downloader *self = (struct downloader *)ctx;

	if (self->data_cb) {
		self->data_cb(data, datasize, self->data_cb_ctx);
	}
}

static void on_response(const struct http_client_response *response, void *ctx)
{
	(void)ctx;
	debug("HTTP status code=%u", response->status_code);
}

static int do_process(struct downloader *self)
{
	struct http_client_request req = {
		.method = HTTP_GET,
	};

	return http_client_request(self->handle, &req, on_response, self);
}

static int do_prepare(struct downloader *self,
		const struct downloader_param *param,
		downloader_data_cb_t data_cb, void *cb_ctx)
{
	memcpy(&self->param, param, sizeof(self->param));
	self->data_cb = data_cb;
	self->data_cb_ctx = cb_ctx;

	/* TODO: inject tls context when needed */
	struct http_client_param http_param = {
		.url = self->param.url,
		.timeout_ms = self->param.timeout_ms,
		.buffer_size = self->param.buffer_size,
	};

	if ((self->handle = http_client_create(&http_param,
			on_data_reception, self)) == NULL) {
		error("Failed to create HTTP client.");
		return -ENOMEM;
	}

	return 0;
}

static int do_cancel(struct downloader *self)
{
	(void)self;
	return 0;
}

static void do_delete(struct downloader *self)
{
	if (self->handle) {
		http_client_destroy(self->handle);
		memset(self, 0, sizeof(*self));
	}
}

struct downloader *downloader_http_create(void)
{
	static struct downloader downloader;

	downloader.api = (struct downloader_api) {
		.prepare = do_prepare,
		.process = do_process,
		.cancel = do_cancel,
		.terminate = do_delete,
	};

	return &downloader;
}
