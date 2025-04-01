/*
 * This file is part of PAZZK EVSE project <https://pazzk.net/>.
 * Copyright (c) 2024 Kyunghwan Kwon <k@libmcu.org>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "net/http_client.h"
#include <errno.h>
#include "libmcu/compiler.h"

struct http_client {
	http_client_data_cb_t data_cb;
	void *data_cb_ctx;
};

int http_client_request(struct http_client *self,
		const struct http_client_request *req,
		http_client_response_cb_t cb, void *cb_ctx)
{
	unused(self);
	unused(req);
	unused(cb);
	unused(cb_ctx);
	return -ENOTSUP;
}

struct http_client *http_client_create(const struct http_client_param *param,
		http_client_data_cb_t cb, void *cb_ctx)
{
	unused(param);
	unused(cb);
	unused(cb_ctx);
	return NULL;
}

int http_client_destroy(struct http_client *self)
{
	unused(self);
	return -ENOTSUP;
}
