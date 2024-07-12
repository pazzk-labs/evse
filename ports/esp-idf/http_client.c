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
#include <stdlib.h>
#include "esp_http_client.h"
#include "logger.h"

#define MIN(a, b)		(((a) > (b))? (b) : (a))

struct http_client {
	esp_http_client_handle_t handle;
	http_client_data_cb_t data_cb;
	void *data_cb_ctx;
	int error;
};

static esp_err_t on_http_event(esp_http_client_event_t *evt)
{
	struct http_client *client = (struct http_client *)evt->user_data;

	switch (evt->event_id) {
	case HTTP_EVENT_DISCONNECTED:
		debug("HTTP_EVENT_DISCONNECTED");
		break;
	case HTTP_EVENT_ON_CONNECTED:
		debug("HTTP_EVENT_ON_CONNECTED");
		break;
	case HTTP_EVENT_ON_HEADER:
		debug("key=%s, value=%s", evt->header_key, evt->header_value);
		break;
	case HTTP_EVENT_ON_DATA:
		if (client->data_cb) {
			(*client->data_cb)(evt->data, evt->data_len,
					client->data_cb_ctx);
		}
		break;
	case HTTP_EVENT_ON_FINISH:
		debug("HTTP_EVENT_ON_FINISH");
		break;
	case HTTP_EVENT_ERROR: /* fall through */
		client->error = -EFAULT;
		break;
	case HTTP_EVENT_HEADER_SENT: /* fall through */
	default:
		break;
      }

	return ESP_OK;
}

int http_client_request(struct http_client *self,
		const struct http_client_request *req,
		http_client_response_cb_t cb, void *cb_ctx)
{
	esp_http_client_set_method(self->handle,
			(esp_http_client_method_t)req->method);

	esp_err_t err = esp_http_client_perform(self->handle);

	if (err == ESP_ERR_HTTP_EAGAIN && self->error == 0) {
		return -EINPROGRESS;
	} else if (err != ESP_OK || self->error) {
		error("HTTP request failure: %x %x", err, self->error);
		self->error = 0;
		return -ECONNABORTED;
	}

	const struct http_client_response response = {
		.status_code = (uint16_t)
			esp_http_client_get_status_code(self->handle),
	};

	if (cb) {
		(*cb)(&response, cb_ctx);
	}

	return 0;
}

struct http_client *http_client_create(const struct http_client_param *param,
		http_client_data_cb_t cb, void *cb_ctx)
{
	struct http_client *client = malloc(sizeof(struct http_client));

	if (!client) {
		return NULL;
	}

	const esp_http_client_config_t config = {
		.event_handler = on_http_event,
		.url = param->url,
		.timeout_ms = param->timeout_ms,
		.cert_pem = (const char *)param->ca_cert,
		.client_cert_pem = (const char *)param->client_cert,
		.client_key_pem = (const char *)param->client_key,
		.buffer_size = param->buffer_size,
		.buffer_size_tx = param->buffer_size,
		.is_async = true,
		.user_data = client,
	};

	client->data_cb = cb;
	client->data_cb_ctx = cb_ctx;
	client->error = 0;

	if ((client->handle = esp_http_client_init(&config)) == NULL) {
		free(client);
		return NULL;
	}

	return client;
}

int http_client_destroy(struct http_client *self)
{
	esp_err_t err = esp_http_client_close(self->handle);
	err |= esp_http_client_cleanup(self->handle);

	free(self);

	return err;
}
