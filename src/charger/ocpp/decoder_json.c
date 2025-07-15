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

#include "decoder_json.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "cJSON.h"
#include "libmcu/metrics.h"
#include "libmcu/timext.h"
#include "ocpp/strconv.h"
#include "logger.h"

#if !defined(MIN)
#define MIN(a, b)		((a) > (b)? (b) : (a))
#endif

#if !defined(ARRAY_COUNT)
#define ARRAY_COUNT(x)		(sizeof(x) / sizeof((x)[0]))
#endif

typedef int (*decoder_handler_t)(struct ocpp_message *msg, cJSON *json);

struct decoder_entry {
	const ocpp_message_t msg_type;
	const decoder_handler_t handler;
};

/* NOTE: The message allocated by this function should be freed by the caller.
 * Free will be done in ocpp event callback function when
 * OCPP_EVENT_MESSAGE_FREE or OCPP_EVENT_MESSAGE_INCOMING event is received. */
static void *create_message(struct ocpp_message *msg, size_t msgsize)
{
	void *p = calloc(1, msgsize);

	msg->payload.fmt.data = p;
	msg->payload.size = msgsize;

	if (p) {
		metrics_increase(OCPPMessageAllocCount);
	} else {
		metrics_increase(OCPPMessageAllocFailCount);
	}

	return p;
}

static int do_error(struct ocpp_message *msg, cJSON *json)
{
	unused(msg);
	unused(json);
	return -ENOTSUP;
}

static int do_empty(struct ocpp_message *msg, cJSON *json)
{
	unused(msg);
	unused(json);
	return 0;
}

static int do_current_time(struct ocpp_message *msg, cJSON *json)
{
	const cJSON *datetime = cJSON_GetObjectItem(json, "currentTime");

	if (!datetime) {
		metrics_increase(OCPPMessageFormatInvalidCount);
		return -EBADMSG;
	}

	struct ocpp_Heartbeat_conf *p = (struct ocpp_Heartbeat_conf *)
		create_message(msg, sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	p->currentTime = iso8601_convert_to_time(cJSON_GetStringValue(datetime));

	return 0;
}

static int do_authorize(struct ocpp_message *msg, cJSON *json)
{
	const cJSON *id = cJSON_GetObjectItem(json, "idTagInfo");
	const cJSON *status = cJSON_GetObjectItem(id, "status");
	const cJSON *expiry = cJSON_GetObjectItem(id, "expiryDate");
	const cJSON *pid = cJSON_GetObjectItem(id, "parentIdTag");

	if (!id || !status) {
		metrics_increase(OCPPMessageFormatInvalidCount);
		return -EBADMSG;
	}

	struct ocpp_Authorize_conf *p = (struct ocpp_Authorize_conf *)
		create_message(msg, sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	p->idTagInfo.status =
		ocpp_get_auth_status_from_string(cJSON_GetStringValue(status));

	if (pid) {
		strcpy(p->idTagInfo.parentIdTag, cJSON_GetStringValue(pid));
	}

	if (expiry) {
		p->idTagInfo.expiryDate = iso8601_convert_to_time(
			cJSON_GetStringValue(expiry));
	}

	return 0;
}

static int do_bootnotification(struct ocpp_message *msg, cJSON *json)
{
	const cJSON *datetime = cJSON_GetObjectItem(json, "currentTime");
	const cJSON *interval = cJSON_GetObjectItem(json, "interval");
	const cJSON *status = cJSON_GetObjectItem(json, "status");

	if (!datetime || !interval || !status) {
		metrics_increase(OCPPMessageFormatInvalidCount);
		return -EBADMSG;
	}

	struct ocpp_BootNotification_conf *p =
		(struct ocpp_BootNotification_conf *)
		create_message(msg, sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	p->currentTime = iso8601_convert_to_time(cJSON_GetStringValue(datetime));
	double t = cJSON_GetNumberValue(interval);
	p->interval = (int)t;
	p->status = ocpp_get_boot_status_from_string(cJSON_GetStringValue(status));

	return 0;
}

static int do_change_availability(struct ocpp_message *msg, cJSON *json)
{
	const cJSON *connector = cJSON_GetObjectItem(json, "connectorId");
	const cJSON *type = cJSON_GetObjectItem(json, "type");
	ocpp_availability_t value;

	if (!connector || !type) {
		metrics_increase(OCPPMessageFormatInvalidCount);
		return -EBADMSG;
	}

	if (strcmp(cJSON_GetStringValue(type), "Inoperative") == 0) {
		value = OCPP_INOPERATIVE;
	} else if (strcmp(cJSON_GetStringValue(type), "Operative") == 0) {
		value = OCPP_OPERATIVE;
	} else {
		return -EBADMSG;
	}

	struct ocpp_ChangeAvailability *p = (struct ocpp_ChangeAvailability *)
		create_message(msg, sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	double t = cJSON_GetNumberValue(connector);
	p->connectorId = (int)t;
	p->type = value;

	return 0;
}

static int do_change_configuration(struct ocpp_message *msg, cJSON *json)
{
	const cJSON *key = cJSON_GetObjectItem(json, "key");
	const cJSON *value = cJSON_GetObjectItem(json, "value");

	if (!key || !value) {
		metrics_increase(OCPPMessageFormatInvalidCount);
		return -EBADMSG;
	}

	struct ocpp_ChangeConfiguration *p = (struct ocpp_ChangeConfiguration *)
		create_message(msg, sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	strcpy(p->key, cJSON_GetStringValue(key));
	strcpy(p->value, cJSON_GetStringValue(value));

	return 0;
}

static int do_get_configuration(struct ocpp_message *msg, cJSON *json)
{
	const cJSON *keys = cJSON_GetObjectItem(json, "key");
	const int nkeys = keys? cJSON_GetArraySize(keys) : 0;
	const cJSON *key;
	size_t keys_len = 0;
	size_t i = 0;

	if (nkeys == 0) {
		return 0;
	}

	cJSON_ArrayForEach(key, keys) {
		keys_len += strlen(cJSON_GetStringValue(key)) + 1/*comma*/;
	}

	struct ocpp_GetConfiguration *p = (struct ocpp_GetConfiguration *)
		create_message(msg, sizeof(*p) + keys_len + 1/*null*/);

	if (!p) {
		return -ENOMEM;
	}

	cJSON_ArrayForEach(key, keys) {
		int written = snprintf(&p->keys[i], keys_len - i, "%s%s",
				i? "," : "", cJSON_GetStringValue(key));
		if (written > 0) {
			i += (size_t)written;
		}
	}

	return 0;
}

static int do_remote_start(struct ocpp_message *msg, cJSON *json)
{
	const cJSON *cid = cJSON_GetObjectItem(json, "connectorId");
	const cJSON *uid = cJSON_GetObjectItem(json, "idTag");
	const cJSON *profile = cJSON_GetObjectItem(json, "chargingProfile");

	if (!uid) {
		metrics_increase(OCPPMessageFormatInvalidCount);
		return -EBADMSG;
	}

	struct ocpp_RemoteStartTransaction *p =
		(struct ocpp_RemoteStartTransaction *)
		create_message(msg, sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	strcpy(p->idTag, cJSON_GetStringValue(uid));

	if (cid) {
		double t = cJSON_GetNumberValue(cid);
		p->connectorId = (int)t;
	}
	if (profile) {
		/* TODO: parse charging profile */
	}

	return 0;
}

static int do_remote_stop(struct ocpp_message *msg, cJSON *json)
{
	const cJSON *tid = cJSON_GetObjectItem(json, "transactionId");

	if (!tid) {
		metrics_increase(OCPPMessageFormatInvalidCount);
		return -EBADMSG;
	}

	struct ocpp_RemoteStopTransaction *p =
		(struct ocpp_RemoteStopTransaction *)
		create_message(msg, sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	double t = cJSON_GetNumberValue(tid);
	p->transactionId = (int)t;

	return 0;
}

static int do_reset(struct ocpp_message *msg, cJSON *json)
{
	const cJSON *type = cJSON_GetObjectItem(json, "type");

	if (!type) {
		metrics_increase(OCPPMessageFormatInvalidCount);
		return -EBADMSG;
	}

	struct ocpp_Reset *p = (struct ocpp_Reset *)
		create_message(msg, sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	if (strcmp(cJSON_GetStringValue(type), "Hard") == 0) {
		p->type = OCPP_RESET_HARD;
	} else {
		p->type = OCPP_RESET_SOFT;
	}

	return 0;
}

static int do_start_transaction(struct ocpp_message *msg, cJSON *json)
{
	const cJSON *uid = cJSON_GetObjectItem(json, "idTagInfo");
	const cJSON *tid = cJSON_GetObjectItem(json, "transactionId");
	const cJSON *status = cJSON_GetObjectItem(uid, "status");
	const cJSON *expiry = cJSON_GetObjectItem(uid, "expiryDate");
	const cJSON *pid = cJSON_GetObjectItem(uid, "parentIdTag");

	if (!uid || !tid || !status) {
		metrics_increase(OCPPMessageFormatInvalidCount);
		return -EBADMSG;
	}

	struct ocpp_StartTransaction_conf *p =
		(struct ocpp_StartTransaction_conf *)
		create_message(msg, sizeof(*p));
	const char *s = cJSON_GetStringValue(status);

	if (!p) {
		return -ENOMEM;
	}

	p->idTagInfo.status = ocpp_get_auth_status_from_string(s);
	double t = cJSON_GetNumberValue(tid);
	p->transactionId = (int)t;

	if (expiry) {
		p->idTagInfo.expiryDate = iso8601_convert_to_time(
			cJSON_GetStringValue(expiry));
	}
	if (pid) {
		strcpy(p->idTagInfo.parentIdTag, cJSON_GetStringValue(pid));
	}

	return 0;
}

static int do_updatefirmware(struct ocpp_message *msg, cJSON *json)
{
	const cJSON *location = cJSON_GetObjectItem(json, "location");
	const cJSON *retries = cJSON_GetObjectItem(json, "retries");
	const cJSON *due = cJSON_GetObjectItem(json, "retrieveDate");
	const cJSON *interval = cJSON_GetObjectItem(json, "retryInterval");

	if (!location || !due) {
		metrics_increase(OCPPMessageFormatInvalidCount);
		return -EBADMSG;
	}

	struct ocpp_UpdateFirmware *p = (struct ocpp_UpdateFirmware *)
		create_message(msg, sizeof(*p));

	if (!p) {
		return -ENOMEM;
	}

	strcpy(p->url, cJSON_GetStringValue(location));
	p->retrieveDate = iso8601_convert_to_time(cJSON_GetStringValue(due));

	if (interval) {
		double t = cJSON_GetNumberValue(interval);
		p->retryInterval = (int)t;
	}
	if (retries) {
		double t = cJSON_GetNumberValue(retries);
		p->retries = (int)t;
	}

	return 0;
}

static struct decoder_entry decoders[] = {
	{ OCPP_MSG_AUTHORIZE,                do_authorize },
	{ OCPP_MSG_BOOTNOTIFICATION,         do_bootnotification },
	{ OCPP_MSG_CHANGE_AVAILABILITY,      do_change_availability },
	{ OCPP_MSG_CHANGE_CONFIGURATION,     do_change_configuration },
	{ OCPP_MSG_CLEAR_CACHE,              do_empty },
	{ OCPP_MSG_DIAGNOSTICS_NOTIFICATION, do_empty },
	{ OCPP_MSG_FIRMWARE_NOTIFICATION,    do_empty },
	{ OCPP_MSG_GET_CONFIGURATION,        do_get_configuration },
	{ OCPP_MSG_HEARTBEAT,                do_current_time },
	{ OCPP_MSG_METER_VALUES,             do_empty },
	{ OCPP_MSG_REMOTE_START_TRANSACTION, do_remote_start },
	{ OCPP_MSG_REMOTE_STOP_TRANSACTION,  do_remote_stop },
	{ OCPP_MSG_RESET,                    do_reset },
	{ OCPP_MSG_START_TRANSACTION,        do_start_transaction },
	{ OCPP_MSG_STATUS_NOTIFICATION,      do_empty },
	{ OCPP_MSG_STOP_TRANSACTION,         do_empty },
	{ OCPP_MSG_UPDATE_FIRMWARE,          do_updatefirmware },
};

static decoder_handler_t decoder(ocpp_message_t msgtype)
{
	for (size_t i = 0; i < ARRAY_COUNT(decoders); i++) {
		if (decoders[i].msg_type == msgtype) {
			return decoders[i].handler;
		}
	}

	error("No decoder found for %s", ocpp_stringify_type(msgtype));
	return &do_error;
}

static int decode(struct ocpp_message *msg, cJSON *json)
{
	const int narr = cJSON_GetArraySize(json);

	if (narr < 3) {
		return -EPROTO;
	}

	const double t = cJSON_GetNumberValue(cJSON_GetArrayItem(json, 0));
	const ocpp_message_role_t role = (ocpp_message_role_t)t;
	const char *id = cJSON_GetStringValue(cJSON_GetArrayItem(json, 1));
	cJSON *payload = cJSON_GetArrayItem(json, narr - 1);
	ocpp_message_t type = ocpp_get_type_from_idstr(id);

	if (role < OCPP_MSG_ROLE_CALL || role > OCPP_MSG_ROLE_CALLERROR ||
			(role == OCPP_MSG_ROLE_CALL && payload == NULL)) {
		metrics_increase(OCPPMessageFormatInvalidCount);
		return -EBADMSG;
	} else if (role == OCPP_MSG_ROLE_CALL) {
		const char *typestr =
			cJSON_GetStringValue(cJSON_GetArrayItem(json, 2));
		type = ocpp_get_type_from_string(typestr);
	}

	*msg = (struct ocpp_message) {
		.role = role,
		.type = type,
	};
	memcpy(msg->id, id, MIN(sizeof(msg->id), strlen(id)));

	return (*decoder(type))(msg, payload);
}

int decoder_json_decode(struct ocpp_message *msg, const char *json_str,
		const size_t json_strlen, size_t *decoded_len)
{
	const char *pnext;
	cJSON *json = cJSON_ParseWithLengthOpts(json_str,
			json_strlen, &pnext, false);
	size_t consumed = json_strlen; /* drop the message if failed */
	int err = -EINVAL;

	if (!json || !json_strlen) {
		if (!json_strlen) {
			err = -ENOMSG;
		}
		goto out;
	}

	consumed = (size_t)(pnext - json_str);
	err = decode(msg, json);
out:
	cJSON_Delete(json);
	if (decoded_len) {
		*decoded_len = consumed;
	}
	return err;
}
