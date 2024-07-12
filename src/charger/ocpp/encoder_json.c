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

#include "encoder_json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "ocpp/strconv.h"
#include "libmcu/timext.h"
#include "libmcu/strext.h"
#include "logger.h"
#include "ocpp/type.h"

#define MSG_IDSTR_MAXLEN		64

#if !defined(ARRAY_SIZE)
#define ARRAY_SIZE(x)			(sizeof(x) / sizeof((x)[0]))
#endif

typedef bool (*encoder_handler_t)(const struct ocpp_message *msg, cJSON *json);

struct encoder_entry {
	const ocpp_message_t msg_type;
	const encoder_handler_t handler;
};

struct configuration_ctx {
	cJSON *known;
	cJSON *unknown;
	int unknown_count;
};

static bool do_empty(const struct ocpp_message *msg, cJSON *json)
{
	(void)msg;
	(void)json;
	return true;
}

static bool do_authorize(const struct ocpp_message *msg, cJSON *json)
{
	const struct ocpp_Authorize *p =
		(const struct ocpp_Authorize *)msg->payload.fmt.request;

	return cJSON_AddItemToObject(json,
			"idTag", cJSON_CreateString(p->idTag));
}

static bool do_bootnotification(const struct ocpp_message *msg, cJSON *json)
{
	const struct ocpp_BootNotification *p =
		(const struct ocpp_BootNotification *)msg->payload.fmt.request;
	int expected_ok = 2;
	int ok = 0;

	ok += cJSON_AddItemToObject(json, "chargePointModel",
			cJSON_CreateString(p->chargePointModel));
	ok += cJSON_AddItemToObject(json, "chargePointVendor",
			cJSON_CreateString(p->chargePointVendor));

	if (strlen(p->firmwareVersion) > 0) {
		ok += cJSON_AddItemToObject(json, "firmwareVersion",
				cJSON_CreateString(p->firmwareVersion));
		expected_ok++;
	}
	if (strlen(p->chargePointSerialNumber) > 0) {
		ok += cJSON_AddItemToObject(json, "chargePointSerialNumber",
				cJSON_CreateString(p->chargePointSerialNumber));
		expected_ok++;
	}

	if (ok != expected_ok) {
		return false;
	}

	return true;
}

static bool do_change_availability(const struct ocpp_message *msg, cJSON *json)
{
	const struct ocpp_ChangeAvailability_conf *p =
		(const struct ocpp_ChangeAvailability_conf *)
		msg->payload.fmt.response;

	if (!cJSON_AddItemToObject(json, "status", cJSON_CreateString(
			ocpp_stringify_availability_status(p->status)))) {
		return false;
	}

	return true;
}

static bool do_change_configuration(const struct ocpp_message *msg, cJSON *json)
{
	const struct ocpp_ChangeConfiguration_conf *p =
		(const struct ocpp_ChangeConfiguration_conf *)
		msg->payload.fmt.response;

	if (!cJSON_AddItemToObject(json, "status", cJSON_CreateString(
			ocpp_stringify_config_status(p->status)))) {
		return false;
	}

	return true;
}

static bool do_clear_cache(const struct ocpp_message *msg, cJSON *json)
{
	const struct ocpp_ClearCache_conf *p =
		(const struct ocpp_ClearCache_conf *)msg->payload.fmt.response;

	if (!cJSON_AddItemToObject(json, "status", cJSON_CreateString(
			ocpp_stringify_remote_status(p->status)))) {
		return false;
	}

	return true;
}

static bool do_diagnostic_noti(const struct ocpp_message *msg, cJSON *json)
{
	const struct ocpp_DiagnosticsStatusNotification *p =
		(const struct ocpp_DiagnosticsStatusNotification *)
		msg->payload.fmt.request;

	const char *str = ocpp_stringify_comm_status(p->status);
	cJSON_AddItemToObject(json, "status", cJSON_CreateString(str));

	return true;
}

static bool do_fw_staus_noti(const struct ocpp_message *msg, cJSON *json)
{
	const struct ocpp_FirmwareStatusNotification *p =
		(const struct ocpp_FirmwareStatusNotification *)
		msg->payload.fmt.request;

	const char *str = ocpp_stringify_comm_status(p->status);
	cJSON_AddItemToObject(json, "status", cJSON_CreateString(str));

	return true;
}

static void pack_csl_connector_phase_rotation(cJSON *json, const int value)
{
	if (value < 0 || value > OCPP_PHASE_ROTATION_TSR) {
		return;
	}

	const char *str[] = {
		[OCPP_PHASE_ROTATION_NOT_APPLICABLE] = "NotApplicable",
		[OCPP_PHASE_ROTATION_UNKNOWN] = "Unknown",
		[OCPP_PHASE_ROTATION_RST] = "RST",
		[OCPP_PHASE_ROTATION_RTS] = "RTS",
		[OCPP_PHASE_ROTATION_SRT] = "SRT",
		[OCPP_PHASE_ROTATION_STR] = "STR",
		[OCPP_PHASE_ROTATION_TRS] = "TRS",
		[OCPP_PHASE_ROTATION_TSR] = "TSR",
	};

	cJSON_AddItemToObject(json, "value", cJSON_CreateString(str[value]));
}

static void pack_csl_meter_data(cJSON *json, const int value)
{
	const size_t maxlen = 512;
	char *buf;

	if ((buf = malloc(maxlen)) == NULL) {
		return;
	}

	if (ocpp_stringify_measurand(buf, maxlen, (ocpp_measurand_t)value)) {
		cJSON_AddItemToObject(json, "value", cJSON_CreateString(buf));
	}

	free(buf);
}

static void pack_csl_profile(cJSON *json, const int value)
{
	char buf[128] = { 0, };
	if (ocpp_stringify_profile(buf, sizeof(buf), (ocpp_profile_t)value)) {
		cJSON_AddItemToObject(json, "value", cJSON_CreateString(buf));
	}
}

static void pack_csl_charging_rate_unit(cJSON *json, const int value)
{
	/* TODO: Implement */
}

static void pack_configuration_value(cJSON *json, const char *key,
		const uint8_t *value, const size_t value_size)
{
	const struct {
		const char *key;
		void (*fn)(cJSON *json, const int value);
	} csl_handler[] = {
		{ "ConnectorPhaseRotation", pack_csl_connector_phase_rotation },
		{ "MeterValuesAlignedData", pack_csl_meter_data },
		{ "MeterValuesSampledData", pack_csl_meter_data },
		{ "StopTxnAlignedData", pack_csl_meter_data },
		{ "StopTxnSampledData", pack_csl_meter_data },
		{ "SupportedFeatureProfiles", pack_csl_profile },
		{ "ChargingScheduleAllowedChargingRateUnit", pack_csl_charging_rate_unit },
	};
	const ocpp_configuration_data_t value_type =
		ocpp_get_configuration_data_type(key);
	int ival;

	if (value_type == OCPP_CONF_TYPE_STR) {
		cJSON_AddItemToObject(json, "value",
				cJSON_CreateString((const char *)value));
	} else if (value_type == OCPP_CONF_TYPE_BOOL) {
		char buf[8] = { 0, };
		snprintf(buf, sizeof(buf), "%s",
				*(const bool *)value? "true" : "false");
		cJSON_AddItemToObject(json, "value", cJSON_CreateString(buf));
	} else if (value_type == OCPP_CONF_TYPE_CSL) {
		for (size_t i = 0; i < ARRAY_SIZE(csl_handler); i++) {
			if (strcmp(key, csl_handler[i].key) == 0) {
				memcpy(&ival, value, sizeof(ival));
				csl_handler[i].fn(json, ival);
				break;
			}
		}
	} else {
		char buf[16] = { 0, };
		memcpy(&ival, value, sizeof(ival));
		snprintf(buf, sizeof(buf), "%d", ival);
		cJSON_AddItemToObject(json, "value", cJSON_CreateString(buf));
	}
}

static bool pack_configuration(const char *key, cJSON *known, cJSON *unknown)
{
	const size_t value_size = ocpp_get_configuration_size(key);
	uint8_t value[value_size];
	bool readonly;
	cJSON *obj;

	if (value_size == 0) {
		cJSON_AddItemToArray(unknown, cJSON_CreateString(key));
		return false;
	}

	if (!ocpp_is_configuration_readable(key) ||
			(obj = cJSON_CreateObject()) == NULL) {
		return false;
	}

	ocpp_get_configuration(key, value, value_size, &readonly);

	cJSON_AddItemToArray(known, obj);
	cJSON_AddItemToObject(obj, "key", cJSON_CreateString(key));
	cJSON_AddItemToObject(obj, "readonly", cJSON_CreateBool(readonly));

	pack_configuration_value(obj, key, value, value_size);

	return true;
}

static int pack_all_configurations(cJSON *known, cJSON *unknown, int max_count)
{
	int count = 0;

	for (size_t i = 0; i < ocpp_count_configurations(); i++) {
		const char *key;

		if (!(key = ocpp_get_configuration_keystr_from_index((int)i))) {
			continue;
		}

		count++;
		pack_configuration(key, known, unknown);
	}

	return count;
}

static void on_each_conf_key(const char *chunk, size_t chunk_len, void *ctx)
{
	struct configuration_ctx *p = (struct configuration_ctx *)ctx;
	char key[chunk_len+1];
	memset(key, 0, chunk_len+1);
	strncpy(key, chunk, chunk_len);
	strtrim(key, ' ');

	if (chunk_len && !pack_configuration(key, p->known, p->unknown)) {
		p->unknown_count++;
	}
}

static int pack_configurations(const char *keys, cJSON *known, cJSON *unknown,
		int max_count, int *unknown_count)
{
	struct configuration_ctx ctx = {
		.known = known,
		.unknown = unknown,
		.unknown_count = 0,
	};

	int count = (int)strchunk(keys, ',', on_each_conf_key, &ctx);

	if (!count) {
		count = pack_all_configurations(known, unknown, max_count);
	}

	return count;
}

static bool do_get_configuration(const struct ocpp_message *msg, cJSON *json)
{
	const char *keystr = msg->payload.fmt.data;
	int unknown_count = 0;
	int max_count = 0;
	cJSON *known;
	cJSON *unknown;

	if ((known = cJSON_CreateArray()) == NULL) {
		return false;
	}
	if ((unknown = cJSON_CreateArray()) == NULL) {
		cJSON_Delete(known);
		return false;
	}

	ocpp_get_configuration("GetConfigurationMaxKeys",
			&max_count, sizeof(max_count), NULL);

	int count = pack_configurations(keystr,
			known, unknown, max_count, &unknown_count);

	if (count - unknown_count > 0) {
		cJSON_AddItemToObject(json, "configurationKey", known);
	} else {
		cJSON_Delete(known);
	}

	if (unknown_count) {
		cJSON_AddItemToObject(json, "unknownKey", unknown);
	} else {
		cJSON_Delete(unknown);
	}

	return true;
}

static cJSON *create_sample_value(const struct ocpp_SampledValue *sample)
{
	cJSON *value = cJSON_CreateObject();

	if (!value) {
		return NULL;
	}

	cJSON_AddItemToObject(value, "value", cJSON_CreateString(sample->value));

	if (sample->context) {
		cJSON_AddItemToObject(value, "context", cJSON_CreateString(
				ocpp_stringify_context(sample->context)));
	}
	if (sample->measurand) {
		char buf[32] = { 0, };
		ocpp_stringify_measurand(buf, sizeof(buf), sample->measurand);
		cJSON_AddItemToObject(value, "measurand", cJSON_CreateString(buf));
	}
	if (sample->unit) {
		cJSON_AddItemToObject(value, "unit", cJSON_CreateString(
				ocpp_stringify_unit(sample->unit)));
	}

	return value;
}

static cJSON *create_meter_value(const struct ocpp_MeterValue *p, size_t n)
{
	cJSON *value = cJSON_CreateObject();

	if (!value) {
		return NULL;
	}

	char tstr[32] = { 0, };
	iso8601_convert_to_string(p->timestamp, tstr, sizeof(tstr));
	cJSON_AddItemToObject(value, "timestamp", cJSON_CreateString(tstr));

	cJSON *samples = cJSON_CreateArray();
	if (samples) {
		cJSON_AddItemToObject(value, "sampledValue", samples);

		for (size_t i = 0; i < n; i++) {
			cJSON *sample = create_sample_value(&p->sampledValue[i]);
			if (sample) {
				cJSON_AddItemToArray(samples, sample);
			}
		}
	}

	return value;
}

static bool do_meter_value(const struct ocpp_message *msg, cJSON *json)
{
	const struct ocpp_MeterValues *p =
		(const struct ocpp_MeterValues *)msg->payload.fmt.request;
        /* FIXME: deal with multiple meter values and sampled values. only one
         * meter value with multiple sampled values supported at the moment. */
        const size_t n = (msg->payload.size - sizeof(*p))
		/ sizeof(p->meterValue.sampledValue[0]);
	int expected_ok = 1;
	int ok = 0;

	ok += cJSON_AddItemToObject(json, "connectorId",
			cJSON_CreateNumber(p->connectorId));
	if (p->transactionId) {
		ok += cJSON_AddItemToObject(json, "transactionId",
				cJSON_CreateNumber(p->transactionId));
		expected_ok++;
	}

	cJSON *values = cJSON_CreateArray();

	if (values) {
		ok += cJSON_AddItemToObject(json, "meterValue", values);
		expected_ok++;
		cJSON *value = create_meter_value(&p->meterValue, n);
		if (value) {
			ok += cJSON_AddItemToArray(values, value);
			expected_ok++;
		}
	}

	if (ok != expected_ok) {
		return false;
	}

	return true;
}

static bool do_remote_start_stop(const struct ocpp_message *msg, cJSON *json)
{
	const struct ocpp_RemoteStartTransaction_conf *p =
		(const struct ocpp_RemoteStartTransaction_conf *)
		msg->payload.fmt.response;

	if (!cJSON_AddItemToObject(json, "status", cJSON_CreateString(
			ocpp_stringify_remote_status(p->status)))) {
		return false;
	}

	return true;
}

static bool do_reset(const struct ocpp_message *msg, cJSON *json)
{
	const struct ocpp_Reset_conf *p = (const struct ocpp_Reset_conf *)
		msg->payload.fmt.response;

	if (!cJSON_AddItemToObject(json, "status", cJSON_CreateString(
			ocpp_stringify_remote_status(p->status)))) {
		return false;
	}

	return true;
}

static bool do_start_transaction(const struct ocpp_message *msg, cJSON *json)
{
	const struct ocpp_StartTransaction *p =
		(const struct ocpp_StartTransaction *)msg->payload.fmt.request;
	int expected_ok = 4;
	int ok = 0;
	char tstr[32] = { 0, };

	ok += cJSON_AddItemToObject(json, "connectorId",
			cJSON_CreateNumber(p->connectorId));
	ok += cJSON_AddItemToObject(json, "meterStart",
			cJSON_CreateNumber((double)p->meterStart));
	ok += cJSON_AddItemToObject(json, "idTag",
			cJSON_CreateString(p->idTag));

	iso8601_convert_to_string(p->timestamp, tstr, sizeof(tstr));
	ok += cJSON_AddItemToObject(json, "timestamp", cJSON_CreateString(tstr));

	if (p->reservationId) {
		ok += cJSON_AddItemToObject(json, "reservationId",
				cJSON_CreateNumber(p->reservationId));
		expected_ok++;
	}

	if (ok != expected_ok) {
		return false;
	}

	return true;
}

static bool do_statusnotification(const struct ocpp_message *msg, cJSON *json)
{
	const struct ocpp_StatusNotification *p =
		(const struct ocpp_StatusNotification *)
		msg->payload.fmt.request;
	int expected_ok = 3;
	int ok = 0;

	ok += cJSON_AddItemToObject(json, "connectorId",
			cJSON_CreateNumber(p->connectorId));
	ok += cJSON_AddItemToObject(json, "errorCode",
			cJSON_CreateString(ocpp_stringify_error(p->errorCode)));
	ok += cJSON_AddItemToObject(json, "status",
			cJSON_CreateString(ocpp_stringify_status(p->status)));

	if (strlen(p->info) > 0) {
		ok += cJSON_AddItemToObject(json, "info",
				cJSON_CreateString(p->info));
		expected_ok++;
	}
	if (strlen(p->vendorId) > 0) {
		ok += cJSON_AddItemToObject(json, "vendorId",
				cJSON_CreateString(p->vendorId));
		expected_ok++;
	}
	if (strlen(p->vendorErrorCode) > 0) {
		ok += cJSON_AddItemToObject(json, "vendorErrorCode",
				cJSON_CreateString(p->vendorErrorCode));
		expected_ok++;
	}
	if (p->timestamp) {
		char tstr[32] = { 0, };
		iso8601_convert_to_string(p->timestamp, tstr, sizeof(tstr));
		ok += cJSON_AddItemToObject(json, "timestamp",
				cJSON_CreateString(tstr));
		expected_ok++;
	}

	if (ok != expected_ok) {
		return false;
	}

	return true;
}

static bool do_stop_transaction(const struct ocpp_message *msg, cJSON *json)
{
	const struct ocpp_StopTransaction *p =
		(const struct ocpp_StopTransaction *)msg->payload.fmt.request;
	int expected_ok = 4;
	int ok = 0;
	char tstr[32] = { 0, };

	ok += cJSON_AddItemToObject(json, "transactionId",
			cJSON_CreateNumber(p->transactionId));
	ok += cJSON_AddItemToObject(json, "meterStop",
			cJSON_CreateNumber((double)p->meterStop));
	ok += cJSON_AddItemToObject(json, "reason", cJSON_CreateString(
			ocpp_stringify_stop_reason(p->reason)));

	iso8601_convert_to_string(p->timestamp, tstr, sizeof(tstr));
	ok += cJSON_AddItemToObject(json, "timestamp", cJSON_CreateString(tstr));

	if (p->idTag[0] != '\0') {
		ok += cJSON_AddItemToObject(json, "idTag",
				cJSON_CreateString(p->idTag));
		expected_ok++;
	}

	/* TODO: implemet transactionData */

	if (ok != expected_ok) {
		return false;
	}

	return true;
}

static struct encoder_entry encoders[] = {
	{ OCPP_MSG_AUTHORIZE,                do_authorize },
	{ OCPP_MSG_BOOTNOTIFICATION,         do_bootnotification },
	{ OCPP_MSG_CHANGE_AVAILABILITY,      do_change_availability },
	{ OCPP_MSG_CHANGE_CONFIGURATION,     do_change_configuration },
	{ OCPP_MSG_CLEAR_CACHE,              do_clear_cache },
	{ OCPP_MSG_DIAGNOSTICS_NOTIFICATION, do_diagnostic_noti },
	{ OCPP_MSG_FIRMWARE_NOTIFICATION,    do_fw_staus_noti },
	{ OCPP_MSG_GET_CONFIGURATION,        do_get_configuration },
	{ OCPP_MSG_HEARTBEAT,                do_empty },
	{ OCPP_MSG_METER_VALUES,             do_meter_value },
	{ OCPP_MSG_REMOTE_START_TRANSACTION, do_remote_start_stop },
	{ OCPP_MSG_REMOTE_STOP_TRANSACTION,  do_remote_start_stop },
	{ OCPP_MSG_RESET,                    do_reset },
	{ OCPP_MSG_START_TRANSACTION,        do_start_transaction },
	{ OCPP_MSG_STATUS_NOTIFICATION,      do_statusnotification },
	{ OCPP_MSG_STOP_TRANSACTION,         do_stop_transaction },
	{ OCPP_MSG_UPDATE_FIRMWARE,          do_empty },
};

static bool encode(const struct ocpp_message *msg, cJSON *json)
{
	if (msg->role == OCPP_MSG_ROLE_CALLERROR) {
		return do_empty(msg, json);
	}

	for (size_t i = 0; i < ARRAY_SIZE(encoders); i++) {
		if (encoders[i].msg_type == msg->type) {
			return encoders[i].handler(msg, json);
		}
	}

	error("No encoder found for message type %s",
			ocpp_stringify_type(msg->type));
	return false;
}

static cJSON *create_message_header(const ocpp_message_role_t role,
		const char *id, const ocpp_message_t type)
{
	int expected_ok = 2;
	int ok = 0;
	cJSON *json = cJSON_CreateArray();

	if (json) {
		ok += cJSON_AddItemToArray(json, cJSON_CreateNumber(role));
		ok += cJSON_AddItemToArray(json, cJSON_CreateString(id));

		if (role == OCPP_MSG_ROLE_CALL) {
			const char *typestr = ocpp_stringify_type(type);
			char buf[MSG_IDSTR_MAXLEN] = { 0, };
			snprintf(buf, sizeof(buf), "%s", typestr);

			ok += cJSON_AddItemToArray(json,
					cJSON_CreateString(buf));
			expected_ok++;
		} else if (role == OCPP_MSG_ROLE_CALLERROR) {
			ok += cJSON_AddItemToArray(json,
					cJSON_CreateString("NotImplemented"));
			ok += cJSON_AddItemToArray(json,
					cJSON_CreateString(""));
			expected_ok += 2;
		}

		if (ok != expected_ok) {
			cJSON_Delete(json);
			json = NULL;
			error("Failed to create message header");
		}
	}

	return json;
}

static cJSON *create_json_container(const struct ocpp_message *msg,
		cJSON **body)
{
	cJSON *json = create_message_header(msg->role, msg->id, msg->type);
	cJSON *payload = cJSON_CreateObject();

	if (json && payload && cJSON_AddItemToArray(json, payload)) {
		*body = payload;
		return json;
	}

	cJSON_Delete(payload); /* safe to free null pointer */
	cJSON_Delete(json);
	error("Failed to create JSON container for %s",
			ocpp_stringify_type(msg->type));

	return NULL;
}

char *encoder_json_encode(const struct ocpp_message *msg, size_t *msglen)
{
	size_t len = 0;
	char *text = NULL;
	cJSON *payload = NULL;
	cJSON *json;

	if (!msg) {
		error("Invalid message");
		return NULL;
	}

	if ((json = create_json_container(msg, &payload)) && payload &&
			encode(msg, payload)) {
		text = cJSON_PrintUnformatted(json);

		if (text) {
			len = strlen(text);
		}
	}

	if (msglen) {
		*msglen = len;
	}

	cJSON_Delete(json); /* safe to free null pointer */

	return text;
}

void encoder_json_free(char *json)
{
	cJSON_free(json);
}
