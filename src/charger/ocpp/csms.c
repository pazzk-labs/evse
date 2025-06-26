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
#include <string.h>
#include <errno.h>

#include "ocpp/core/configuration_size.h"
#include "ocpp/version.h"
#include "libmcu/metrics.h"
#include "libmcu/compiler.h"

#include "net/server_ws.h"
#include "net/util.h"
#include "adapter.h"
#include "handler.h"
#include "config.h"
#include "secret.h"
#include "logger.h"

#define RXQUEUE_SIZE			4096

#define DEFAULT_WS_PING_INTERVAL_SEC	300
#define DEFAULT_TX_TIMEOUT_MS		8000
#define DEFAULT_SERVER_URL		"wss://ocpp.pazzk.net:9000"

static_assert(sizeof(((struct config *)0)->net.server_url) == WS_URI_MAXLEN,
		"WS_URI_MAXLEN is not equal to the size of ws_param.url");
static_assert(sizeof(((struct config *)0)->net.server_id) == WS_AUTH_ID,
		"WS_AUTH_ID is not equal to the size of ws_param.auth.id");
static_assert(sizeof(((struct config *)0)->net.server_pass) == WS_AUTH_PASS,
		"WS_AUTH_PASS is not equal to the size of ws_param.auth.pass");
static_assert(sizeof(((struct config *)0)->ocpp.config)
		== OCPP_CONFIGURATION_TOTAL_SIZE,
		"OCPP configuration size mismatch");

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

	config_get("net.server.url", param.url, sizeof(param.url));
	config_get("net.server.ping", &param.ping_interval_sec,
			sizeof(param.ping_interval_sec));
	config_get("net.server.id", param.auth.id, sizeof(param.auth.id));
	config_get("net.server.pass", param.auth.pass, sizeof(param.auth.pass));

	if (net_is_secure_protocol(net_get_protocol_from_url(param.url))) {
		/* NOTE: The allocated dynamic memory is not freed because the
		 * certificate is used throughout the device's lifecycle.
		 * To minimize RAM usage, consider accessing the flash memory
		 * directly. */
		uint8_t *key = (uint8_t *)malloc(CONFIG_X509_MAXLEN);
		uint8_t *cert = (uint8_t *)malloc(CONFIG_X509_MAXLEN);
		uint8_t *ca = (uint8_t *)malloc(CONFIG_X509_MAXLEN);
		int len;

		if (key && (len = secret_read(SECRET_KEY_X509_KEY,
				key, CONFIG_X509_MAXLEN)) > 0) {
			param.tls.key_len = (size_t)len;
			param.tls.key = key;
		}
		if (cert && config_get("x509.cert", cert,
				CONFIG_X509_MAXLEN) == 0) {
			param.tls.cert_len = strnlen((const char *)cert,
					CONFIG_X509_MAXLEN);
			param.tls.cert = cert;
		}
		if (ca && config_get("x509.ca", ca, CONFIG_X509_MAXLEN) == 0) {
			param.tls.ca_len = strnlen((const char *)ca,
					CONFIG_X509_MAXLEN);
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
	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;
	return adapter_push_request(oc, msgtype, opt);
}

int csms_request_defer(const ocpp_message_t msgtype, void *ctx, void *opt,
		const uint32_t delay_sec)
{
	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;
	return adapter_push_request_defer(oc, msgtype, opt, delay_sec);
}

int csms_response(const ocpp_message_t msgtype,
		const struct ocpp_message *req, void *ctx, void *opt)
{
	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;
	return adapter_push_response(oc, msgtype, req, opt);
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
	const uint32_t ocpp_version = MAKE_VERSION(OCPP_VERSION_MAJOR,
			OCPP_VERSION_MINOR,
			OCPP_VERSION_BUILD);
	uint32_t ocpp_version_saved;
	config_get("ocpp.version", &ocpp_version_saved,
			sizeof(ocpp_version_saved));
	if (ocpp_version != ocpp_version_saved) {
		error("OCPP version mismatch: %x != %x",
				ocpp_version, ocpp_version_saved);
		config_set("ocpp.version", &ocpp_version, sizeof(ocpp_version));
	}

	int err = initialize_server();

	if (!err) {
		const size_t len = ocpp_compute_configuration_size();
		void *p = calloc(1, len);
		if (p == NULL) {
			return -ENOMEM;
		}

		if (!config_is_zeroed("ocpp.config")) {
			if (config_get("ocpp.config", p, len) == 0) {
				ocpp_copy_configuration_from(p, len);
			}
		}

		free(p);

		adapter_init(csms, RXQUEUE_SIZE);
		err = ocpp_init(on_ocpp_event, ctx);
	}

	return err;
}
