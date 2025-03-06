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

#include "csms.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "libmcu/metrics.h"

#include "net/server_ws.h"
#include "net/util.h"
#include "adapter.h"
#include "handler.h"
#include "config.h"
#include "secret.h"
#include "logger.h"

#define RXQUEUE_SIZE			4096
#define PKA_BUFSIZE			2048

#define DEFAULT_WS_PING_INTERVAL_SEC	300
#define DEFAULT_TX_TIMEOUT_MS		10000
#define DEFAULT_SERVER_URL		"wss://csms.pazzk.net"

static struct server *csms;

static int initialize_server(void)
{
	struct ws_param param = {
		.url = DEFAULT_SERVER_URL,
		.header = "Sec-WebSocket-Protocol: ocpp1.6\r\n",
		.write_timeout_ms = DEFAULT_TX_TIMEOUT_MS,
		.rxq_maxsize = RXQUEUE_SIZE,
		.ping_interval_sec = DEFAULT_WS_PING_INTERVAL_SEC,
	};

	config_read(CONFIG_KEY_SERVER_URL, param.url, sizeof(param.url)-1);
	config_read(CONFIG_KEY_WS_PING_INTERVAL, &param.ping_interval_sec,
			sizeof(param.ping_interval_sec));
	config_read(CONFIG_KEY_SERVER_ID, param.auth.id,
			sizeof(param.auth.id)-1);
	config_read(CONFIG_KEY_SERVER_PASS, param.auth.pass,
			sizeof(param.auth.pass)-1);

	if (net_is_secure_protocol(net_get_protocol_from_url(param.url))) {
		/* NOTE: The allocated dynamic memory is not freed because the
		 * certificate is used throughout the device's lifecycle.
		 * To minimize RAM usage, consider accessing the flash memory
		 * directly. */
		uint8_t *key = (uint8_t *)malloc(PKA_BUFSIZE);
		uint8_t *cert = (uint8_t *)malloc(PKA_BUFSIZE);
		uint8_t *ca = (uint8_t *)malloc(PKA_BUFSIZE);
		int len;

		if (key && (len = secret_read(SECRET_KEY_X509_KEY,
				key, PKA_BUFSIZE)) > 0) {
			param.tls.key_len = (size_t)len;
			param.tls.key = key;
		}
		if (cert && (len = config_read(CONFIG_KEY_X509_CERT,
				cert, PKA_BUFSIZE)) > 0) {
			param.tls.cert_len = (size_t)len;
			param.tls.cert = cert;
		}
		if (ca && (len = config_read(CONFIG_KEY_X509_CA,
				ca, PKA_BUFSIZE)) > 0) {
			param.tls.ca_len = (size_t)len;
			param.tls.ca = ca;
		}
	}

	info("CSMS URL: %s", param.url);

	if (!(csms = ws_create_server(&param, NULL, NULL))) {
		return -ENOMEM;
	}

	return 0;
}

static void on_ocpp_event(ocpp_event_t event_type,
		const struct ocpp_message *message, void *ctx)
{
	struct ocpp_charger *charger = (struct ocpp_charger *)ctx;

	if (event_type == OCPP_EVENT_MESSAGE_OUTGOING) {
		return;
	} else if (event_type == OCPP_EVENT_MESSAGE_INCOMING) {
		if (!message->payload.fmt.data) {
			metrics_increase(OCPPMessageNullPayloadCount);
			debug("no payload(%d): %s", event_type,
					ocpp_stringify_type(message->type));
		}
		handler_process(charger, message);
	}

        /* NOTE: incoming messages must be freed too because it is allocated by
         * decoder. */
	if (message->payload.fmt.data) {
		free(message->payload.fmt.data);
		metrics_increase(OCPPMessageFreeCount);
	}
}

bool csms_is_up(void)
{
	return server_connected(csms);
}

int csms_request(const ocpp_message_t msgtype, void *ctx, void *opt)
{
	return adapter_push_request(ctx, msgtype, opt);
}

int csms_request_defer(const ocpp_message_t msgtype, void *ctx, void *opt,
		const uint32_t delay_sec)
{
	return adapter_push_request_defer(ctx, msgtype, opt, delay_sec);
}

int csms_response(const ocpp_message_t msgtype,
		const struct ocpp_message *req, void *ctx, void *opt)
{
	return adapter_push_response(ctx, msgtype, req, opt);
}

int csms_reconnect(const uint32_t delay_sec)
{
	(void)delay_sec;

	int err = 0;

	if (server_connected(csms)) {
		err |= server_disable(csms);
		err |= server_enable(csms);
	}

	return err;
}

int csms_init(void *ctx)
{
	int err = initialize_server();

	if (!err) {
		const size_t len = ocpp_compute_configuration_size();
		void *p = (void *)calloc(1, len);
		if (p == NULL) {
			return -ENOMEM;
		}

		if (config_read(CONFIG_KEY_OCPP_CONFIG, p, len) == (int)len) {
			ocpp_copy_configuration_from(p, len);
		} else {
			warn("Failed to load OCPP configuration");
		}

		free(p);

		adapter_init(csms, RXQUEUE_SIZE);
		err = ocpp_init(on_ocpp_event, ctx);
	}

	return err;
}
