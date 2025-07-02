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

#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <libwebsockets.h>
#include <time.h>

#include "libmcu/compiler.h"
#include "libmcu/ringbuf.h"
#include "libmcu/base64.h"
#include "net/util.h"
#include "logger.h"

#define DEFAULT_RECONNECT_INTERVAL_SEC	5
#if !defined(URL_MAXLEN)
#define URL_MAXLEN			128
#endif

struct ws_server {
	struct server_api api;
	struct ws_param param;

	struct lws_context *context;
	struct lws *wsi;

	struct lws_context_creation_info ctx_info;
	struct lws_client_connect_info conn_info;

	struct ringbuf *rxq;
	pthread_t thread;

	ws_receive_cb_t cb;
	void *cb_ctx;

	char host[URL_MAXLEN];
	char path[URL_MAXLEN];
	char header[WS_HEADER_MAXLEN];

	time_t last_reconnect;
	bool is_connected;
};

static void add_basic_auth_header(struct ws_server *ws,
		struct lws *wsi, void *in, size_t in_len)
{
	if (ws->param.auth.id[0] == '\0') {
		return;
	}

	char b[WS_AUTH_ID + WS_AUTH_PASS + 2];
	const int b_len = snprintf(b, sizeof(b), "%s:%s",
			ws->param.auth.id, ws->param.auth.pass);
	const int len = snprintf(ws->header, sizeof(ws->header), "Basic ");
	const size_t needed = (((size_t)b_len + 2) / 3) * 4;
	if (len > 0 && b_len > 0 && (size_t)len + needed < sizeof(ws->header)) {
		const size_t cap = sizeof(ws->header) - (size_t)len;
		const size_t n = lm_base64_encode(&ws->header[len], cap,
				b, (size_t)b_len);
		ws->header[(size_t)len + n] = '\0';

		uint8_t **p = (uint8_t **)in;
		uint8_t *end = (*p) + in_len;
		if (lws_add_http_header_by_name(wsi,
				  (const unsigned char *)"Authorization:",
				  (const unsigned char *)ws->header,
				  (int)strlen(ws->header), p, end) < 0) {
			error("failed to add Authorization header");
		}
	}
}

static int on_ws_event(struct lws *wsi, enum lws_callback_reasons reason,
		void *user, void *in, size_t len)
{
	unused(user);

	struct ws_server *ws = (struct ws_server *)
		lws_context_user(lws_get_context(wsi));

	switch (reason) {
	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		ws->is_connected = true;
		info("connection established");
		break;
	case LWS_CALLBACK_CLIENT_RECEIVE:
		if (ringbuf_write(ws->rxq, in, len) != len) {
			error("ringbuf_write failed");
		}

		if (ws->cb) {
			ws->cb(ws, in, len, ws->cb_ctx);
		}
		break;
	case LWS_CALLBACK_CLIENT_CLOSED:
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		ws->is_connected = false;
		info("connection closed");
		break;
	case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
		add_basic_auth_header(ws, wsi, in, len);
		break;
	default:
		break;
	}

	return 0;
}

static struct lws_protocols protocols[] = {
	{
		.name = "ocpp1.6",
		.callback = on_ws_event,
	},
	{ 0, },
};

static int send_data(struct server *srv,
		const void *data, const size_t datasize)
{
	struct ws_server *ws = (struct ws_server *)srv;

	if (!ws->is_connected) {
		return -ENOTCONN;
	}

	unsigned char buf[LWS_PRE + 4096];

	if (datasize > sizeof(buf) - LWS_PRE) {
		return -EMSGSIZE;
	}

	memcpy(&buf[LWS_PRE], data, datasize);

	int n = lws_write(ws->wsi, &buf[LWS_PRE], datasize, LWS_WRITE_TEXT);
	return (n < 0) ? -EIO : n;
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

static int connect_to_server(struct server *srv)
{
	struct ws_server *ws = (struct ws_server *)srv;
	struct lws_context_creation_info *ctxcfg = &ws->ctx_info;

	memset(ctxcfg, 0, sizeof(*ctxcfg));
	ctxcfg->port = CONTEXT_PORT_NO_LISTEN;
	ctxcfg->protocols = protocols;
	ctxcfg->options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
	ctxcfg->user = ws;
	ctxcfg->client_ssl_key_mem = ws->param.tls.key;
	ctxcfg->client_ssl_key_mem_len = (unsigned int)ws->param.tls.key_len;
	ctxcfg->client_ssl_cert_mem = ws->param.tls.cert;
	ctxcfg->client_ssl_cert_mem_len = (unsigned int)ws->param.tls.cert_len;
	ctxcfg->client_ssl_ca_mem = ws->param.tls.ca;
	ctxcfg->client_ssl_ca_mem_len = (unsigned int)ws->param.tls.ca_len;

	if (!(ws->context = lws_create_context(ctxcfg))) {
		return -ENOMEM;
	}

	memset(&ws->conn_info, 0, sizeof(ws->conn_info));
	ws->conn_info.address = ws->host;
	ws->conn_info.path = ws->path;
	ws->conn_info.context = ws->context;
	ws->conn_info.port = (int)net_get_port_from_url(ws->param.url);
	ws->conn_info.host = ws->conn_info.address;
	ws->conn_info.origin = "origin";
	ws->conn_info.protocol = protocols[0].name;
	ws->conn_info.pwsi = &ws->wsi;

	if (net_is_secure_protocol(net_get_protocol_from_url(ws->param.url))) {
		ws->conn_info.ssl_connection =
			LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED;
	}

	info("connecting to %s:%d%s", ws->host, ws->conn_info.port, ws->path);

	if (!(ws->wsi = lws_client_connect_via_info(&ws->conn_info))) {
		lws_context_destroy(ws->context);
		error("failed to connect");
		return -EIO;
	}

	return 0;
}

static int disconnect_from_server(struct server *srv)
{
	struct ws_server *ws = (struct ws_server *)srv;

	lws_context_destroy(ws->context);
	ws->is_connected = false;

	return 0;
}

static bool connected(const struct server *srv)
{
	const struct ws_server *ws = (const struct ws_server *)srv;
	return ws->is_connected;
}

static int enable(struct server *srv)
{
	unused(srv);
	return 0;
}

static int disable(struct server *srv)
{
	unused(srv);
	return 0;
}

static void *ws_thread(void *arg)
{
	struct ws_server *ws = (struct ws_server *)arg;

	net_get_host_from_url(ws->param.url, ws->host, sizeof(ws->host));
	net_get_path_from_url(ws->param.url, ws->path, sizeof(ws->path));

	while (1) {
		lws_service(ws->context, 0);

		if (!ws->is_connected) {
			const time_t now = time(NULL);

			if (now - ws->last_reconnect
					>= DEFAULT_RECONNECT_INTERVAL_SEC) {
				ws->last_reconnect = now;
				connect_to_server((struct server *)ws);
			}
		}
	}

	return NULL;
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
	ws.rxq = ringbuf_create(ws.param.rxq_maxsize);
	ws.cb = cb;
	ws.cb_ctx = cb_ctx;

	pthread_create(&ws.thread, NULL, ws_thread, &ws);

	return (struct server *)&ws;
}

void ws_delete_server(struct ws_server *ws)
{
	lws_service(ws->context, 0);
	unused(ws);
}
