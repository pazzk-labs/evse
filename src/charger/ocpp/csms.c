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

#include "libmcu/ringbuf.h"
#include "libmcu/metrics.h"

#include "net/server_ws.h"
#include "net/util.h"
#include "encoder_json.h"
#include "decoder_json.h"
#include "messages.h"
#include "handler.h"
#include "config.h"
#include "secret.h"
#include "logger.h"

#define RXQUEUE_SIZE			4096
#define BUFSIZE				2048
#define PKA_BUFSIZE			2048

#define DEFAULT_WS_PING_INTERVAL_SEC	300
#define DEFAULT_TX_TIMEOUT_MS		10000
#define DEFAULT_SERVER_URL		"wss://csms.pazzk.net"

static struct server *csms;
static struct ringbuf *rxq;

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

/* A time-based message ID is used by default rather than a UUID.
 *
 * Time-based ID was chosen because:
 * 1. shorter message length than UUID.
 * 2. additional information beyond the ID(time difference or order between
 *    messages).
 * 3. intuitive
 * 4. ease of implementation
 *
 * Note that the main premise is that you don't need a unique ID for every
 * message on every device, but only need to maintain a unique ID within a
 * single device. However, if considering distributed systems or addressing
 * predictability-related security vulnerabilities, it is recommended to use
 * a UUID-based approach. */
static void generate_message_id_by_time(char *buf, size_t bufsize)
{
	static uint8_t nonce;
	const time_t ts = time(NULL);
	snprintf(buf, bufsize, "%d-%03u", (int32_t)ts, (uint8_t)nonce++);
}

void ocpp_generate_message_id(void *buf, size_t bufsize)
{
#if defined(USE_UUID_MESSAGE_ID)
	generate_message_id_by_uuid(buf, bufsize);
#else
	generate_message_id_by_time(buf, bufsize);
#endif
}

int ocpp_send(const struct ocpp_message *msg)
{
	size_t json_len = 0;
	char *json = encoder_json_encode(msg, &json_len);

	if (!json || !json_len) {
		return -ENOMEM;
	}

	int err = server_send(csms, json, json_len);

	if (err > 0) { /* success */
		err = 0;
		metrics_increase(OCPPMessageSentCount);
		debug("%.*s", (int)json_len, json);
	}

	encoder_json_free(json);

	info("%s message(%d): %s, %s",
			(err == 0)? "Sent" : "Failed to send",
			err, ocpp_stringify_type(msg->type), msg->id);

	return err;
}

int ocpp_recv(struct ocpp_message *msg)
{
	static uint8_t buf[BUFSIZE];
	size_t cap = ringbuf_capacity(rxq) - ringbuf_length(rxq);

	while (cap > sizeof(buf)) {
		int len = server_recv(csms, buf, sizeof(buf));
		if (len <= 0) {
			break;
		}

		ringbuf_write(rxq, buf, (size_t)len);
		cap -= (size_t)len;
	}

	size_t decoded_len = 0;
	const size_t len = ringbuf_peek(rxq, 0, buf, sizeof(buf));
	int err = decoder_json_decode(msg, (char *)buf, len, &decoded_len);
	ringbuf_consume(rxq, decoded_len);

	if (!err || err == -ENOTSUP) {
		debug("%.*s", (int)decoded_len, buf);
		info("Received %zu bytes, decoded %zu bytes", len, decoded_len);
	}

	return err;
}

bool csms_is_up(void)
{
	return server_connected(csms);
}

bool csms_is_configuration_reboot_required(const char *key)
{
	return false;
}

int csms_request(const ocpp_message_t msgtype, void *ctx, void *opt)
{
	return message_push_request(ctx, msgtype, opt);
}

int csms_request_defer(const ocpp_message_t msgtype, void *ctx, void *opt,
		const uint32_t delay_sec)
{
	return message_push_request_defer(ctx, msgtype, opt, delay_sec);
}

int csms_response(const ocpp_message_t msgtype,
		const struct ocpp_message *req, void *ctx, void *opt)
{
	return message_push_response(ctx, msgtype, req, opt);
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
	struct ws_param param = {
		.url = DEFAULT_SERVER_URL,
		.header = "Sec-WebSocket-Protocol: ocpp1.6\r\n",
		.write_timeout_ms = DEFAULT_TX_TIMEOUT_MS,
		.rxq_maxsize = RXQUEUE_SIZE,
		.ping_interval_sec = DEFAULT_WS_PING_INTERVAL_SEC,
	};

	if (!(rxq = ringbuf_create(RXQUEUE_SIZE))) {
		return -ENOMEM;
	}

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

	return ocpp_init(on_ocpp_event, ctx);
}
