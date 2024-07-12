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

#ifndef SERVER_WEBSOCKET_H
#define SERVER_WEBSOCKET_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "net/server.h"

#if !defined(WS_URI_MAXLEN)
#define WS_URI_MAXLEN			256
#endif
#if !defined(WS_AUTH_ID)
#define WS_AUTH_ID			32
#endif
#if !defined(WS_AUTH_PASS)
#define WS_AUTH_PASS			40 /* the maximum of ocpp1.6 */
#endif
#if !defined(WS_HEADER_MAXLEN)
#define WS_HEADER_MAXLEN		256
#endif

struct ws_param {
	char url[WS_URI_MAXLEN]; /* includes protocol, host, port and path.
		e.g. "wss://example.com:443/collector/v1/PZK1202411190001" */
	char header[WS_HEADER_MAXLEN];

	struct {
		char id[WS_AUTH_ID];
		char pass[WS_AUTH_PASS+1];
	} auth;

	struct {
		const uint8_t *cert;
		size_t cert_len;
		const uint8_t *key;
		size_t key_len;
		const uint8_t *ca;
		size_t ca_len;
	} tls;

	uint32_t write_timeout_ms;
	uint32_t ping_interval_sec;

	size_t rxq_maxsize; /* Maximum size of the receive queue in bytes. */

	/* This is a flag to indicate whether the server should be started and
	 * stopped manually. If set to true, the server will not start or stop
	 * automatically and must be controlled manually by the user. This can
	 * be useful for scenarios where the server's lifecycle needs to be
	 * managed explicitly. If set to false, the server will start and stop
	 * automatically based on the network connection status. */
	bool start_stop_manually;
};

struct ws_server;

typedef void (*ws_receive_cb_t)(struct ws_server *srv,
		const void *data, const size_t datasize, void *ctx);

struct server *ws_create_server(const struct ws_param *param,
		ws_receive_cb_t cb, void *cb_ctx);
void ws_delete_server(struct ws_server *srv);

#if defined(__cplusplus)
}
#endif

#endif /* SERVER_WEBSOCKET_H */
