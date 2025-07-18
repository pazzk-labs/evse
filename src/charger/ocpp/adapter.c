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

#include "adapter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

#include "libmcu/metrics.h"
#include "libmcu/board.h"
#include "libmcu/ringbuf.h"
#include "libmcu/assert.h"
#include "libmcu/compiler.h"

#include "charger/ocpp.h"
#include "ocpp_connector_internal.h"
#include "encoder_json.h"
#include "decoder_json.h"
#include "net/server.h"
#include "config.h"
#include "logger.h"

static_assert(sizeof(((struct config *)0)->ocpp.vendor) == OCPP_CiString20,
		"config.ocpp.vendor size mismatch");
static_assert(sizeof(((struct config *)0)->ocpp.model) == OCPP_CiString20,
		"config.ocpp.model size mismatch");

#if !defined(ARRAY_COUNT)
#define ARRAY_COUNT(x)		(sizeof(x) / sizeof((x)[0]))
#endif

#define BUFSIZE			2048

typedef int (*message_handler_t)(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec);

struct message_entry {
	const ocpp_message_t msg_type;
	const message_handler_t handler;
};

static struct server *csms;
static struct ringbuf *rxq;

/* NOTE: The message allocated by this function should be freed by the caller.
 * Free will be done in ocpp event callback function when
 * OCPP_EVENT_MESSAGE_FREE or OCPP_EVENT_MESSAGE_INCOMING event is received. */
static void *alloc_message(const size_t msg_size)
{
	void *p = calloc(1, msg_size);

	if (p) {
		metrics_increase(OCPPMessageAllocCount);
	} else {
		metrics_increase(OCPPMessageAllocFailCount);
	}

	return p;
}

static int request(const ocpp_message_t type, const struct ocpp_message *req,
		const void *data, const size_t datasize,
		const uint32_t delay_sec, const bool force_or_error, void *ctx)
{
	if (req) {
		return ocpp_push_response(req, data, datasize,
				force_or_error, ctx);
	} else if (delay_sec) {
		return ocpp_push_request_defer(type, data, datasize,
				delay_sec, ctx);
	}

	if (force_or_error) {
		return ocpp_push_request_force(type, data, datasize, ctx);
	}

	return ocpp_push_request(type, data, datasize, ctx);
}

static int request_free_if_fail(const ocpp_message_t type,
		const struct ocpp_message *req,
		void *data, const size_t datasize,
		const uint32_t delay_sec, const bool force_or_error, void *ctx)
{
	int err = request(type, req, data, datasize,
			delay_sec, force_or_error, ctx);

	if (err) {
		free(data);
		metrics_increase(OCPPPushFailedCount);
		error("Failed to push message(%d): %s",
				err, ocpp_stringify_type(type));
	}

	return err;
}

static int do_empty(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	unused(ctx);
	return request(msg_type, req, NULL, 0, delay_sec, false, c);
}

static int do_authorize(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	unused(ctx);
	struct ocpp_Authorize *p =
		(struct ocpp_Authorize *)alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	strcpy(p->idTag, (const char *)c->session.auth.trial.uid);

	return request_free_if_fail(msg_type, req, p, sizeof(*p),
			delay_sec, false, c);
}

static int do_bootnotification(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	unused(ctx);
	struct ocpp_BootNotification *p =
		(struct ocpp_BootNotification *)alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	const char *evse_id = board_name();
	*p = (struct ocpp_BootNotification) { 0, };

	if (strlen(evse_id) <= 0) {
		evse_id = board_get_serial_number_string();
	}

	strcpy(p->chargePointSerialNumber, evse_id);
	strcpy(p->firmwareVersion, board_get_version_string());
	config_get("ocpp.vendor", p->chargePointVendor,
			sizeof(p->chargePointVendor));
	config_get("ocpp.model", p->chargePointModel,
			sizeof(p->chargePointModel));

	return request_free_if_fail(msg_type, req, p, sizeof(*p),
			delay_sec, true, c);
}

static int do_change_availability(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_ChangeAvailability_conf *p =
		(struct ocpp_ChangeAvailability_conf *)
		alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	p->status = (ocpp_availability_status_t)(uintptr_t)ctx;

	return request_free_if_fail(msg_type, req, p, sizeof(*p),
			delay_sec, false, c);
}

static int do_change_configuration(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_ChangeConfiguration_conf *p =
		(struct ocpp_ChangeConfiguration_conf *)
		alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	p->status = (ocpp_config_status_t)(uintptr_t)ctx;

	return request_free_if_fail(msg_type, req, p, sizeof(*p),
			delay_sec, false, c);
}

static int do_clear_cache(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_ClearCache_conf *p =
		(struct ocpp_ClearCache_conf *)alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	p->status = (ocpp_remote_status_t)(uintptr_t)ctx;

	return request_free_if_fail(msg_type, req, p, sizeof(*p),
			delay_sec, false, c);
}

static int do_datatransfer(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	unused(ctx);
	const size_t total_size = sizeof(struct ocpp_DataTransfer) +
		c->ocpp_info.datatransfer.datasize;
	struct ocpp_DataTransfer *p =
		(struct ocpp_DataTransfer *)alloc_message(total_size);

	if (!p) {
		return -ENOMEM;
	}

	*p = (struct ocpp_DataTransfer) { 0, };
	config_get("ocpp.vendor", p->vendorId, sizeof(p->vendorId));
	strcpy(p->messageId, c->ocpp_info.datatransfer.id);
	if (c->ocpp_info.datatransfer.data &&
			c->ocpp_info.datatransfer.datasize) {
		memcpy(p->data, c->ocpp_info.datatransfer.data,
				c->ocpp_info.datatransfer.datasize);
	}

	return request_free_if_fail(msg_type, req, p, total_size,
			delay_sec, false, c);
}

static int do_diagnostic_status_noti(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_DiagnosticsStatusNotification *p =
		(struct ocpp_DiagnosticsStatusNotification *)
		alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	*p = (struct ocpp_DiagnosticsStatusNotification) {
		.status = (ocpp_comm_status_t)(uintptr_t)ctx,
	};

	return request_free_if_fail(msg_type, req, p, sizeof(*p),
			delay_sec, false, c);
}

static int do_fw_status_noti(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_FirmwareStatusNotification *p =
		(struct ocpp_FirmwareStatusNotification *)
		alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	*p = (struct ocpp_FirmwareStatusNotification) {
		.status = (ocpp_comm_status_t)(uintptr_t)ctx,
	};

	return request_free_if_fail(msg_type, req, p, sizeof(*p),
			delay_sec, false, c);
}

static int do_get_configuration(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	unused(ctx);
	size_t req_keylen = 0;

	if (req->payload.fmt.request) {
		req_keylen = strlen(((const struct ocpp_GetConfiguration *)
			req->payload.fmt.request)->keys);
	}

	const size_t total_size = req_keylen + 1;
	char *p = (char *)alloc_message(total_size);

	if (!p) {
		return -ENOMEM;
	}

	/* NOTE: An ad-hoc method of passing the request message to
	 * be processed by the encoder. */
	if (req_keylen) {
		strncpy(p, ((const struct ocpp_GetConfiguration *)
			req->payload.fmt.request)->keys, req_keylen);
		p[req_keylen] = '\0';
	}

	return request_free_if_fail(msg_type, req, p, total_size,
			delay_sec, false, c);
}

static size_t count_measurands(ocpp_measurand_t measurands)
{
	size_t count = 0;
	unsigned int t = (unsigned int)measurands;

	while (t) {
		count += t & 1;
		t >>= 1;
	}

	return count;
}

static ocpp_measurand_t pick_measurand_by_index(ocpp_measurand_t measurands,
		const size_t index)
{
	size_t count = 0;

	for (size_t i = 0; i < sizeof(measurands) * 8; i++) {
		if (measurands & (1 << i)) {
			if (++count == index) {
				return (ocpp_measurand_t)(1 << i);
			}
		}
	}

	return (ocpp_measurand_t)0;
}

static ocpp_measure_unit_t get_unit_by_measurand(ocpp_measurand_t measurand)
{
	switch (measurand) {
	case OCPP_MEASURAND_TEMPERATURE:    return OCPP_UNIT_CELSIUS;
	case OCPP_MEASURAND_VOLTAGE:        return OCPP_UNIT_V;
	case OCPP_MEASURAND_CURRENT_IMPORT: return OCPP_UNIT_A;
	case OCPP_MEASURAND_SOC:            return OCPP_UNIT_PERCENT;
	case OCPP_MEASURAND_POWER_FACTOR:   return OCPP_UNIT_PERCENT;
	case OCPP_MEASURAND_FREQUENCY:      return OCPP_UNIT_PERCENT;
	default:
		return OCPP_UNIT_WH;
	}
}

static void set_measurand_value(struct ocpp_SampledValue *p,
		const ocpp_measurand_t measurand,
		const struct session_metering *v)
{
	switch (measurand) {
	case OCPP_MEASURAND_ENERGY_ACTIVE_IMPORT_REGISTER:
		snprintf(p->value, sizeof(p->value), "%"PRIu64, v->wh);
		break;
	case OCPP_MEASURAND_POWER_ACTIVE_IMPORT:
		snprintf(p->value, sizeof(p->value), "%"PRId32, v->watt);
		break;
	case OCPP_MEASURAND_CURRENT_IMPORT:
		snprintf(p->value, sizeof(p->value), "%"PRId32".%03"PRId32,
				v->milliamp / 1000, v->milliamp % 1000);
		break;
	case OCPP_MEASURAND_VOLTAGE:
		snprintf(p->value, sizeof(p->value), "%"PRId32".%03"PRId32,
				v->millivolt / 1000, v->millivolt % 1000);
		break;
	case OCPP_MEASURAND_POWER_FACTOR:
		snprintf(p->value, sizeof(p->value), "%"PRId32".%02"PRId32,
				v->pf_centi / 100, v->pf_centi % 100);
		break;
	case OCPP_MEASURAND_FREQUENCY:
		snprintf(p->value, sizeof(p->value), "%"PRId32".%02"PRId32,
				v->centi_hertz / 100, v->centi_hertz % 100);
		break;
	case OCPP_MEASURAND_TEMPERATURE:
		snprintf(p->value, sizeof(p->value), "%"PRId16".%02"PRId16,
				v->temperature_centi / 100,
				v->temperature_centi % 100);
		break;
	case OCPP_MEASURAND_SOC:
		snprintf(p->value, sizeof(p->value), "%"PRIu8, v->soc_pct);
		break;
	default:
		memset(p->value, 0, sizeof(p->value));
		break;
	}
}

static int do_metervalue(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	const ocpp_measurand_t measurands = (ocpp_measurand_t)(uintptr_t)ctx;
	const size_t n_samples = count_measurands(measurands);
	const size_t total_size = sizeof(struct ocpp_MeterValues) +
		sizeof(struct ocpp_MeterValue) +
		n_samples * sizeof(struct ocpp_SampledValue);
	struct ocpp_MeterValues *p =
		(struct ocpp_MeterValues *)alloc_message(total_size);

	if (!p) {
		return -ENOMEM;
	}

	struct ocpp_MeterValue *value =
		(struct ocpp_MeterValue *)(void *)p->meterValue;
	*p = (struct ocpp_MeterValues) {
		.connectorId = c->base.id,
		.transactionId = (int)c->session.transaction_id,
	};
	*value = (struct ocpp_MeterValue) {
		.timestamp = c->now,
	};
	struct ocpp_SampledValue *sampledValue =
		(struct ocpp_SampledValue *)(void *)value->sampledValue;

	for (size_t i = 0; i < n_samples; i++) {
		ocpp_measurand_t measurand =
			pick_measurand_by_index(measurands, i+1);
		sampledValue[i].context = c->session.metering.context;
		sampledValue[i].measurand = measurand;
		sampledValue[i].unit = get_unit_by_measurand(measurand);

		set_measurand_value(&sampledValue[i],
				measurand, &c->session.metering);
	}

	return request_free_if_fail(msg_type, req, p, total_size,
			delay_sec, false, c);
}

static int do_remote_start_stop(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_RemoteStartTransaction_conf *p =
		(struct ocpp_RemoteStartTransaction_conf *)
		alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	p->status = (ocpp_remote_status_t)(uintptr_t)ctx;

	return request_free_if_fail(msg_type, req, p, sizeof(*p),
			delay_sec, false, c);
}

static int do_reset(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	unused(c);
	struct ocpp_Reset_conf *p =
		(struct ocpp_Reset_conf *)alloc_message(sizeof(*p));
	struct charger *charger = (struct charger *)ctx;

	if (!p) {
		return -ENOMEM;
	}

	if (charger && ocpp_charger_get_pending_reboot_type(charger)
			> OCPP_CHARGER_REBOOT_NONE) {
		p->status = OCPP_REMOTE_STATUS_ACCEPTED;
	} else {
		p->status = OCPP_REMOTE_STATUS_REJECTED;
	}

	return request_free_if_fail(msg_type, req, p, sizeof(*p),
			delay_sec, false, NULL);
}

static int do_starttransaction(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	unused(ctx);
	struct ocpp_StartTransaction *p =
		(struct ocpp_StartTransaction *)alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	*p = (struct ocpp_StartTransaction) {
		.connectorId = c->base.id,
		.meterStart = c->session.metering.start_wh,
		.timestamp = c->now,
		.reservationId = (int)c->session.reservation_id, /* optional */
	};
	memcpy(p->idTag, c->session.auth.current.uid, sizeof(p->idTag));

	return request_free_if_fail(msg_type, req, p, sizeof(*p),
			delay_sec, true, c);
}

static int do_statusnotification(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_StatusNotification *p =
		(struct ocpp_StatusNotification *)alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	*p = (struct ocpp_StatusNotification) {
		.connectorId = c->base.id,
		.errorCode = ocpp_connector_map_error_to_ocpp(c->base.error),
		.timestamp = c->now, /* optional */
	};

	if (ctx == CONNECTOR_0) {
		p->status = ocpp_connector_is_unavailable(c)?
			OCPP_STATUS_UNAVAILABLE : OCPP_STATUS_AVAILABLE;
	} else {
		p->status = ocpp_connector_map_state_to_ocpp(
				ocpp_connector_state(c));
		if (ctx) { /* optional */
			strcpy(p->info, (char *)ctx);
		}
	}

	if (c->ocpp_info.vendor_error_code) { /* optional */
		strcpy(p->vendorErrorCode, c->ocpp_info.vendor_error_code);
		config_get("ocpp.vendor", p->vendorId, sizeof(p->vendorId));
	}

	return request_free_if_fail(msg_type, req, p, sizeof(*p),
			delay_sec, false, c);
}

static int do_stoptransaction(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_StopTransaction *p =
		(struct ocpp_StopTransaction *)alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	*p = (struct ocpp_StopTransaction) {
		.meterStop = c->session.metering.stop_wh, /* optional */
		.timestamp = c->now,
		.transactionId = (int)c->session.transaction_id,
		.reason = (ocpp_stop_reason_t)(uintptr_t)ctx, /* optional */
	};

	if (ocpp_connector_is_session_pending(c)) {
		memcpy(p->idTag, c->session.auth.trial.uid, sizeof(p->idTag));
	} else {
		memcpy(p->idTag, c->session.auth.current.uid, sizeof(p->idTag));
	}

	return request_free_if_fail(msg_type, req, p, sizeof(*p),
			delay_sec, true, c);
}

static int do_unlock(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	unused(ctx);
	struct ocpp_UnlockConnector_conf *p =
		(struct ocpp_UnlockConnector_conf *)alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	*p = (struct ocpp_UnlockConnector_conf) {
		.status = OCPP_UNLOCK_NOT_SUPPORTED,
	};

	return request_free_if_fail(msg_type, req, p, sizeof(*p),
			delay_sec, false, c);
}

static const struct message_entry requests[] = {
	{ OCPP_MSG_AUTHORIZE,                do_authorize },
	{ OCPP_MSG_BOOTNOTIFICATION,         do_bootnotification },
	{ OCPP_MSG_DATA_TRANSFER,            do_datatransfer },
	{ OCPP_MSG_DIAGNOSTICS_NOTIFICATION, do_diagnostic_status_noti },
	{ OCPP_MSG_FIRMWARE_NOTIFICATION,    do_fw_status_noti },
	{ OCPP_MSG_HEARTBEAT,                do_empty },
	{ OCPP_MSG_METER_VALUES,             do_metervalue },
	{ OCPP_MSG_START_TRANSACTION,        do_starttransaction },
	{ OCPP_MSG_STATUS_NOTIFICATION,      do_statusnotification },
	{ OCPP_MSG_STOP_TRANSACTION,         do_stoptransaction },
};

static const struct message_entry responses[] = {
	{ OCPP_MSG_CHANGE_AVAILABILITY,      do_change_availability },
	{ OCPP_MSG_CHANGE_CONFIGURATION,     do_change_configuration },
	{ OCPP_MSG_CLEAR_CACHE,              do_clear_cache },
	{ OCPP_MSG_GET_CONFIGURATION,        do_get_configuration },
	{ OCPP_MSG_REMOTE_START_TRANSACTION, do_remote_start_stop },
	{ OCPP_MSG_REMOTE_STOP_TRANSACTION,  do_remote_start_stop },
	{ OCPP_MSG_RESET,                    do_reset },
	{ OCPP_MSG_UNLOCK_CONNECTOR,         do_unlock },
	{ OCPP_MSG_UPDATE_FIRMWARE,          do_empty },
};

static int push(struct ocpp_connector *connector,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec,
		const struct message_entry *handlers,
		const size_t handlers_count)
{
	for (size_t i = 0; i < handlers_count; i++) {
		if (handlers[i].msg_type == msg_type) {
			return (*handlers[i].handler)(connector,
					msg_type, req, ctx, delay_sec);
		}
	}

	error("Unknown message type: %s", ocpp_stringify_type(msg_type));
	return -ENOENT;
}

static int push_request(struct ocpp_connector *connector,
		const ocpp_message_t msg_type, void *ctx,
		const uint32_t delay_sec)
{
	return push(connector, msg_type, NULL,
			ctx, delay_sec, requests, ARRAY_COUNT(requests));
}

static int push_responses(struct ocpp_connector *connector,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	return push(connector, msg_type, req,
			ctx, delay_sec, responses, ARRAY_COUNT(responses));
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
	snprintf(buf, bufsize, "%" PRId32 "-%03u", (int32_t)ts, nonce++);
}

void ocpp_generate_message_id(void *buf, size_t bufsize)
{
#if defined(USE_UUID_MESSAGE_ID)
	generate_message_id_by_uuid((char *)buf, bufsize);
#else
	generate_message_id_by_time((char *)buf, bufsize);
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

	if (err < 0) {
		warn("Failed to send message(%d): %s, %s",
				err, ocpp_stringify_type(msg->type), msg->id);
	} else { /* success */
		err = 0;
		metrics_increase(OCPPMessageSentCount);
		debug("%.*s", (int)json_len, json);
	}

	encoder_json_free(json);

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

	const size_t len = ringbuf_peek(rxq, 0, buf, sizeof(buf));
	if (len == 0) {
		return -ENOENT;
	}

	size_t decoded_len = 0;
	int err = decoder_json_decode(msg, (char *)buf, len, &decoded_len);
	ringbuf_consume(rxq, decoded_len);

	if (!err || err == -ENOTSUP) {
		debug("%.*s", (int)decoded_len, buf);
	} else {
		warn("Received %zu bytes, decoded %zu bytes", len, decoded_len);
	}

	return err;
}

int adapter_push_request(struct ocpp_connector *connector,
		const ocpp_message_t msg_type, void *ctx)
{
	return push_request(connector, msg_type, ctx, 0);
}

int adapter_push_request_defer(struct ocpp_connector *connector,
		const ocpp_message_t msg_type, void *ctx,
		const uint32_t delay_sec)
{
	return push_request(connector, msg_type, ctx, delay_sec);
}

int adapter_push_response(struct ocpp_connector *connector,
		const ocpp_message_t msg_type,
		const struct ocpp_message *req, void *ctx)
{
	return push_responses(connector, msg_type, req, ctx, 0);
}

void adapter_init(struct server *server, size_t rxqueue_size)
{
	csms = server;
	rxq = ringbuf_create(rxqueue_size);
	assert(csms && rxq);
}
