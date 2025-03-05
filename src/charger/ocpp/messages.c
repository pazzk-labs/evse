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

#include "messages.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

#include "charger/ocpp.h"
#include "ocpp_connector_internal.h"
#include "libmcu/metrics.h"
#include "libmcu/board.h"
#include "logger.h"

#if !defined(CHARGER_VENDOR)
#define CHARGER_VENDOR		"net.pazzk"
#endif
#if !defined(CHARGER_MODEL)
#define CHARGER_MODEL		"EVSE-7S"
#endif

#if !defined(ARRAY_SIZE)
#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))
#endif

typedef int (*message_handler_t)(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec);

struct message_entry {
	const ocpp_message_t msg_type;
	const message_handler_t handler;
};

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
		const uint32_t delay_sec, const bool force_or_error,
		struct ocpp_message **created)
{
	if (req) {
		return ocpp_push_response(req, data, datasize, force_or_error);
	} else if (delay_sec) {
		return ocpp_push_request_defer(type, data, datasize, delay_sec);
	}

	if (force_or_error) {
		return ocpp_push_request_force(type, data, datasize, created);
	}
	return ocpp_push_request(type, data, datasize, created);
}

static int request_free_if_fail(const ocpp_message_t type,
		const struct ocpp_message *req,
		void *data, const size_t datasize,
		const uint32_t delay_sec, const bool force_or_error,
		struct ocpp_message **created)
{
	int err = request(type, req, data, datasize, delay_sec, force_or_error,
			created);

	if (err) {
		free(data);
		metrics_increase(OCPPPushFailedCount);
		error("Failed to push message: %d", err);
	}

	return err;
}

static int do_empty(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	return request(msg_type, req, NULL, 0, delay_sec, false, NULL);
}

static int do_authorize(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_Authorize *p = alloc_message(sizeof(*p));
	struct ocpp_message *created;

	if (!p) {
		return -ENOMEM;
	}

	strcpy(p->idTag, (const char *)c->session.auth.trial.id);

	int err = request_free_if_fail(msg_type,
			req, p, sizeof(*p), delay_sec, false, &created);
	if (!err && created) {
		memcpy(c->ocpp_info.msgid, created->id, sizeof(created->id));
	}
	return err;
}

static int do_bootnotification(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_BootNotification *p = alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	*p = (struct ocpp_BootNotification) {
		.chargePointModel = CHARGER_MODEL,
		.chargePointVendor = CHARGER_VENDOR,
		.firmwareVersion = def2str(VERSION_TAG),
	};

	strcpy(p->chargePointSerialNumber, board_get_serial_number_string());

	return request_free_if_fail(msg_type,
			req, p, sizeof(*p), delay_sec, true, NULL);
}

static int do_change_availability(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_ChangeAvailability_conf *p = alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	p->status = (ocpp_availability_status_t)(uintptr_t)ctx;

	return request_free_if_fail(msg_type,
			req, p, sizeof(*p), delay_sec, false, NULL);
}

static int do_change_configuration(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_ChangeConfiguration_conf *p = alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	p->status = (ocpp_config_status_t)(uintptr_t)ctx;

	return request_free_if_fail(msg_type,
			req, p, sizeof(*p), delay_sec, false, NULL);
}

static int do_clear_cache(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_ClearCache_conf *p = alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	p->status = (ocpp_remote_status_t)(uintptr_t)ctx;

	return request_free_if_fail(msg_type,
			req, p, sizeof(*p), delay_sec, false, NULL);
}

static int do_datatransfer(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	const size_t total_size = sizeof(struct ocpp_DataTransfer) +
		c->ocpp_info.datatransfer.datasize;
	struct ocpp_DataTransfer *p = alloc_message(total_size);

	if (!p) {
		return -ENOMEM;
	}

	*p = (struct ocpp_DataTransfer) {
		.vendorId = CHARGER_VENDOR,
	};
	strcpy(p->messageId, c->ocpp_info.datatransfer.id);
	if (c->ocpp_info.datatransfer.data &&
			c->ocpp_info.datatransfer.datasize) {
		memcpy(p->data, c->ocpp_info.datatransfer.data,
				c->ocpp_info.datatransfer.datasize);
	}

	return request_free_if_fail(msg_type,
			req, p, total_size, delay_sec, false, NULL);
}

static int do_diagnostic_status_noti(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_DiagnosticsStatusNotification *p = alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	*p = (struct ocpp_DiagnosticsStatusNotification) {
		.status = (ocpp_comm_status_t)(uintptr_t)ctx,
	};

	return request_free_if_fail(msg_type,
			req, p, sizeof(*p), delay_sec, false, NULL);
}

static int do_fw_status_noti(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_FirmwareStatusNotification *p = alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	*p = (struct ocpp_FirmwareStatusNotification) {
		.status = (ocpp_comm_status_t)(uintptr_t)ctx,
	};

	return request_free_if_fail(msg_type,
			req, p, sizeof(*p), delay_sec, false, NULL);
}

static int do_get_configuration(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	size_t req_keylen = 0;

	if (req->payload.fmt.request) {
		req_keylen = strlen(req->payload.fmt.request);
	}

	const size_t total_size = req_keylen + 1;
	char *p = alloc_message(total_size);

	if (!p) {
		return -ENOMEM;
	}

	/* NOTE: An ad-hoc method of passing the request message to
	 * be processed by the encoder. */
	if (req_keylen) {
		strcpy(p, req->payload.fmt.data);
	}

	return request_free_if_fail(msg_type,
			req, p, total_size, delay_sec, false, NULL);
}

static size_t count_measurands(ocpp_measurand_t measurands)
{
	size_t count = 0;

	while (measurands) {
		count += measurands & 1;
		measurands >>= 1;
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
				return 1 << i;
			}
		}
	}

	return 0;
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
		snprintf(p->value, sizeof(p->value), "%" PRIu64, v->wh);
		break;
	case OCPP_MEASURAND_POWER_ACTIVE_IMPORT:
		snprintf(p->value, sizeof(p->value), "%d", v->watt);
		break;
	case OCPP_MEASURAND_CURRENT_IMPORT:
		snprintf(p->value, sizeof(p->value), "%d", v->milliamp);
		break;
	case OCPP_MEASURAND_VOLTAGE:
		snprintf(p->value, sizeof(p->value), "%d", v->millivolt);
		break;
	case OCPP_MEASURAND_POWER_FACTOR:
		snprintf(p->value, sizeof(p->value), "%d", v->pf_centi);
		break;
	case OCPP_MEASURAND_FREQUENCY:
		snprintf(p->value, sizeof(p->value), "%d", v->centi_hertz);
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
	const size_t total_size = sizeof(struct ocpp_MeterValues)
		+ n_samples * sizeof(struct ocpp_SampledValue);
	struct ocpp_MeterValues *p = alloc_message(total_size);

	if (!p) {
		return -ENOMEM;
	}

	*p = (struct ocpp_MeterValues) {
		.connectorId = c->base.id,
		.transactionId = (int)c->session.transaction_id,
		.meterValue = {
			.timestamp = c->now,
		},
	};

	for (size_t i = 0; i < n_samples; i++) {
		ocpp_measurand_t measurand = pick_measurand_by_index(measurands, i+1);
		p->meterValue.sampledValue[i].context = c->session.metering.context;
		p->meterValue.sampledValue[i].measurand = measurand;
		p->meterValue.sampledValue[i].unit = get_unit_by_measurand(measurand);

		set_measurand_value(&p->meterValue.sampledValue[i],
				measurand, &c->session.metering);
	}

	return request_free_if_fail(msg_type,
			req, p, total_size, delay_sec, false, NULL);
}

static int do_remote_start_stop(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_RemoteStartTransaction_conf *p = alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	p->status = (ocpp_remote_status_t)(uintptr_t)ctx;

	return request_free_if_fail(msg_type,
			req, p, sizeof(*p), delay_sec, false, NULL);
}

static int do_reset(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_Reset_conf *p = alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	if (ocpp_connector_is_remote_reset_requested(c, NULL)) {
		p->status = OCPP_REMOTE_STATUS_ACCEPTED;
	} else {
		p->status = OCPP_REMOTE_STATUS_REJECTED;
	}

	return request_free_if_fail(msg_type,
			req, p, sizeof(*p), delay_sec, false, NULL);
}

static int do_starttransaction(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_StartTransaction *p = alloc_message(sizeof(*p));
	struct ocpp_message *created;

	if (!p) {
		return -ENOMEM;
	}

	*p = (struct ocpp_StartTransaction) {
		.connectorId = c->base.id,
		.meterStart = c->session.metering.start_wh,
		.timestamp = c->now,
		.reservationId = (int)c->session.reservation_id, /* optional */
	};
	memcpy(p->idTag, c->session.auth.current.id, sizeof(p->idTag));

	int err = request_free_if_fail(msg_type,
			req, p, sizeof(*p), delay_sec, true, &created);
	if (!err && created) {
		memcpy(c->ocpp_info.msgid, created->id, sizeof(created->id));
	}
	return err;
}

static int do_statusnotification(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_StatusNotification *p = alloc_message(sizeof(*p));

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
			strcpy(p->info, ctx);
		}
	}

	if (c->ocpp_info.vendor_error_code) { /* optional */
		strcpy(p->vendorErrorCode, c->ocpp_info.vendor_error_code);
		strcpy(p->vendorId, CHARGER_VENDOR);
	}

	return request_free_if_fail(msg_type,
			req, p, sizeof(*p), delay_sec, false, NULL);
}

static int do_stoptransaction(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_StopTransaction *p = alloc_message(sizeof(*p));
	struct ocpp_message *created;

	if (!p) {
		return -ENOMEM;
	}

	*p = (struct ocpp_StopTransaction) {
		.meterStop = c->session.metering.stop_wh, /* optional */
		.timestamp = c->now,
		.transactionId = (int)c->session.transaction_id,
		.reason = (ocpp_stop_reason_t)(uintptr_t)ctx, /* optional */
	};

	if (ocpp_connector_is_session_trial_id_exist(c)) {
		memcpy(p->idTag, c->session.auth.trial.id, sizeof(p->idTag));
	} else {
		memcpy(p->idTag, c->session.auth.current.id, sizeof(p->idTag));
	}

	int err = request_free_if_fail(msg_type,
			req, p, sizeof(*p), delay_sec, true, &created);
	if (!err && created) {
		memcpy(c->ocpp_info.msgid, created->id, sizeof(created->id));
	}
	return err;
}

static int do_unlock(struct ocpp_connector *c,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	struct ocpp_UnlockConnector_conf *p = alloc_message(sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	*p = (struct ocpp_UnlockConnector_conf) {
		.status = OCPP_UNLOCK_NOT_SUPPORTED,
	};

	return request_free_if_fail(msg_type,
			req, p, sizeof(*p), delay_sec, false, NULL);
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
			ctx, delay_sec, requests, ARRAY_SIZE(requests));
}

static int push_responses(struct ocpp_connector *connector,
		const ocpp_message_t msg_type, const struct ocpp_message *req,
		void *ctx, const uint32_t delay_sec)
{
	return push(connector, msg_type, req,
			ctx, delay_sec, responses, ARRAY_SIZE(responses));
}

int message_push_request(struct ocpp_connector *connector,
		const ocpp_message_t msg_type, void *ctx)
{
	return push_request(connector, msg_type, ctx, 0);
}

int message_push_request_defer(struct ocpp_connector *connector,
		const ocpp_message_t msg_type, void *ctx,
		const uint32_t delay_sec)
{
	return push_request(connector, msg_type, ctx, delay_sec);
}

int message_push_response(struct ocpp_connector *connector,
		const ocpp_message_t msg_type,
		const struct ocpp_message *req, void *ctx)
{
	return push_responses(connector, msg_type, req, ctx, 0);
}
