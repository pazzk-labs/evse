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

#include "net/server_ws.h"
#include "net/netmgr.h"

#include <string.h>
#include <errno.h>

#include "esp_websocket_client.h"
#include "libmcu/retry.h"
#include "libmcu/board.h"
#include "libmcu/apptmr.h"
#include "libmcu/ringbuf.h"
#include "logger.h"

#define DEFAULT_TIMEOUT_MS		(10U/*sec*/ * 1000)
#define DEFAULT_RECONNECT_TIMEOUT_MS	DEFAULT_TIMEOUT_MS

#define DEFAULT_MAX_RETRY		10U
#define DEFAULT_MAX_BACKOFF_MS		(100U * 1000)
#define DEFAULT_MIN_BACKOFF_MS		DEFAULT_TIMEOUT_MS
#define DEFAULT_MAX_JITTER_MS		DEFAULT_MIN_BACKOFF_MS

#define RESTART_ERROR_COUNT		10

struct ws_server {
	struct server_api api;

	struct ws_param param;
	ws_receive_cb_t cb;
	void *cb_ctx;

	esp_websocket_client_handle_t handle;
	struct retry retry;
	struct apptmr *timer;
	struct ringbuf *rxq;

	uint32_t timestamp;
	int error_count;
	bool enabled;
};

static bool on_websocket_error(void *ctx)
{
	struct ws_server *ws = (struct ws_server *)ctx;

	if (!ws->enabled) {
		error("ws is not enabled.");
		goto out;
	}

	const uint32_t t = board_get_time_since_boot_ms();
	const uint32_t elapsed = t - ws->timestamp;
	uint32_t backoff_ms;

	if (retry_first(&ws->retry)) {
		retry_backoff(&ws->retry, &backoff_ms, board_random());
	} else {
		backoff_ms = retry_get_backoff(&ws->retry);
	}

	if (elapsed < backoff_ms) {
		/* re-register the task to be called after the backoff time. */
		apptmr_stop(ws->timer);
		apptmr_start(ws->timer, backoff_ms - elapsed);
		info("ws retry in %u ms", backoff_ms - elapsed);
		goto out;
	}

	ws->timestamp = t;
	server_disable((struct server *)ws);
	server_enable((struct server *)ws);
	info("websocket restarted.");

	if (retry_backoff(&ws->retry, &backoff_ms, board_random())
			== RETRY_ERROR_EXHAUSTED) {
		/* TODO: notify the user that the max retry is reached
		 * and reboot the system after cleanup. */
		error("ws retry exhausted.");
	}

out:
	return false;
}

static void on_timeout(struct apptmr *timer, void *arg)
{
	struct ws_server *ws = (struct ws_server *)arg;
	netmgr_register_task(on_websocket_error, ws);
}

static void on_net_event(const netmgr_state_t event, void *ctx)
{
	struct ws_server *ws = (struct ws_server *)ctx;

	if (ws->param.start_stop_manually) {
		return;
	}

	switch (event) {
	case NETMGR_STATE_CONNECTED:
		apptmr_stop(ws->timer);
		retry_reset(&ws->retry);
		server_enable((struct server *)ws);
		info("network connected");
		break;
	case NETMGR_STATE_DISCONNECTED:
		apptmr_stop(ws->timer);
		server_disable((struct server *)ws);
		info("network disconnected");
		break;
	default:
		error("unknown network event %d", event);
		break;
	}
}

static void on_ws_event(void *ctx,
		esp_event_base_t base, int32_t event_id, void *event_data)
{
	const esp_websocket_event_data_t *data =
		(esp_websocket_event_data_t *)event_data;
	struct ws_server *ws = (struct ws_server *)ctx;

	switch (event_id) {
	case WEBSOCKET_EVENT_BEGIN:
		info("websocket event begin");
		break;
	case WEBSOCKET_EVENT_CONNECTED:
		apptmr_stop(ws->timer);
		info("websocket event connected");
		break;
	case WEBSOCKET_EVENT_DISCONNECTED:
		info("websocket event disconnected");
		break;
	case WEBSOCKET_EVENT_FINISH: /* fall through */
	case WEBSOCKET_EVENT_ERROR:
		if (++ws->error_count > RESTART_ERROR_COUNT) {
			ws->error_count = 0;
			ws->timestamp = board_get_time_since_boot_ms();
			netmgr_register_task(on_websocket_error, ws);
			error("too many errors, restarting websocket.");
		}
		error("websocket event: %d", event_id);
		break;
	case WEBSOCKET_EVENT_DATA:
		info("ws opcode=%d, len=%u", data->op_code, data->data_len);
		if (data->op_code == 0x08 && data->data_len == 2) {
			info("Received closed message with code=%d",
				256 * data->data_ptr[0] + data->data_ptr[1]);
		} else {
			if (ringbuf_write(ws->rxq, data->data_ptr,
					data->data_len) != data->data_len) {
				error("rxq overflow");
			}

			if (ws->cb) {
				(*ws->cb)(ws, data->data_ptr, data->data_len,
						ws->cb_ctx);
			}
			retry_reset(&ws->retry);
		}
		break;
	case WEBSOCKET_EVENT_CLOSED:
		break;
	default:
		error("unknown websocket event %d", event_id);
		break;
	}
}

static int send_data(struct server *srv,
		const void *data, const size_t datasize)
{
	struct ws_server *ws = (struct ws_server *)srv;

	if (!ws->enabled) {
		return -ENOTCONN;
	}

	return esp_websocket_client_send_text(ws->handle, data, datasize,
			pdMS_TO_TICKS(ws->param.write_timeout_ms));
}

static int recv_data(struct server *srv, void *buf, const size_t bufsize)
{
	struct ws_server *ws = (struct ws_server *)srv;

	if (ringbuf_length(ws->rxq) == 0) {
		return -ENODATA;
	}

	size_t len = ringbuf_read(ws->rxq, 0, buf, bufsize);

	return (int)len;
}

static bool connected(const struct server *srv)
{
	const struct ws_server *ws = (const struct ws_server *)srv;

	return esp_websocket_client_is_connected(ws->handle);
}

static int connect_to_server(struct server *srv)
{
	/* The connection is established automatically when the network is
	 * connected. */
	return -ENOTSUP;
}

static int disconnect_from_server(struct server *srv)
{
	/* The disconnection is done automatically when the network is
	 * disconnected. */
	return -ENOTSUP;
}

static int enable(struct server *srv)
{
	struct ws_server *ws = (struct ws_server *)srv;

	esp_err_t err = esp_websocket_client_start(ws->handle);

	if (err == ESP_OK) {
		ws->enabled = true;
		return 0;
	}

	return -err;
}

static int disable(struct server *srv)
{
	struct ws_server *ws = (struct ws_server *)srv;

	esp_err_t err = esp_websocket_client_close(ws->handle,
			pdMS_TO_TICKS(ws->param.write_timeout_ms));

	ws->enabled = false;
	return err != ESP_OK? -err : 0;
}

static int init(struct ws_server *ws)
{
	esp_websocket_client_config_t conf = {
		.uri = ws->param.url,
		.client_cert = (const char *)ws->param.tls.cert,
		.client_cert_len = ws->param.tls.cert_len,
		.client_key = (const char *)ws->param.tls.key,
		.client_key_len = ws->param.tls.key_len,
		.cert_pem = (const char *)ws->param.tls.ca,
		.headers = ws->param.header,
		.username = ws->param.auth.id,
		.password = ws->param.auth.pass,
		.network_timeout_ms = ws->param.write_timeout_ms,
		.reconnect_timeout_ms = DEFAULT_RECONNECT_TIMEOUT_MS,
		.ping_interval_sec = ws->param.ping_interval_sec?
			ws->param.ping_interval_sec : SIZE_MAX, /* the maximum
			ping interval used if set to 0,
			because no way to disable ping in the library */
		.user_agent = "Pazzk Websocket Client",
	};

	if ((ws->handle = esp_websocket_client_init(&conf)) == NULL) {
		return -ENOMEM;
	}

	esp_err_t err = esp_websocket_register_events(ws->handle,
			WEBSOCKET_EVENT_ANY, on_ws_event, ws);

	return err != ESP_OK? -err : 0;
}

struct server *ws_create_server(const struct ws_param *param,
		ws_receive_cb_t cb, void *cb_ctx)
{
	static struct ws_server ws = {
		.api = {
			.enable = enable,
			.disable = disable,
			.connect = connect_to_server,
			.disconnect = disconnect_from_server,
			.send = send_data,
			.recv = recv_data,
			.connected = connected,
		},
	};

	memcpy(&ws.param, param, sizeof(*param));
	if (param->tls.ca_len) {
		/* cert. strings should be null-terminated */
		ws.param.tls.ca_len += 1;
	}
	if (param->tls.cert_len) {
		ws.param.tls.cert_len += 1;
	}

	ws.rxq = ringbuf_create(ws.param.rxq_maxsize);
	ws.cb = cb;
	ws.cb_ctx = cb_ctx;

	if (ws.param.write_timeout_ms == 0) {
		ws.param.write_timeout_ms = DEFAULT_TIMEOUT_MS;
	}

	if (init(&ws) != 0) {
		return NULL;
	}

	const netmgr_state_mask_t event_mask =
		NETMGR_STATE_CONNECTED | NETMGR_STATE_DISCONNECTED;
	if (netmgr_register_event_cb(event_mask, on_net_event, &ws) != 0) {
		esp_websocket_client_destroy(ws.handle);
		return NULL;
	}

	/* on error, exponential backoff retry */
	retry_new_static(&ws.retry, &(struct retry_param) {
		.max_attempts = DEFAULT_MAX_RETRY,
		.min_backoff_ms = DEFAULT_MIN_BACKOFF_MS,
		.max_backoff_ms = DEFAULT_MAX_BACKOFF_MS,
		.max_jitter_ms = DEFAULT_MAX_JITTER_MS,
	});

	ws.timer = apptmr_create(false, on_timeout, &ws);
	apptmr_enable(ws.timer);

	return (struct server *)&ws;
}

void ws_delete_server(struct ws_server *ws)
{
	esp_websocket_client_destroy(ws->handle);
}
