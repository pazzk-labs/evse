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

#include "handler.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "charger/ocpp.h"
#include "ocpp_connector_internal.h"
#include "csms.h"
#include "app.h"
#include "updater.h"
#include "net/util.h"
#include "downloader_http.h"
#include "logger.h"
#include "libmcu/strext.h"
#include "ocpp/strconv.h"

#if !defined(ARRAY_COUNT)
#define ARRAY_COUNT(x)		(sizeof(x) / sizeof((x)[0]))
#endif

#if !defined(MIN)
#define MIN(a, b)		((a) > (b)? (b) : (a))
#endif

typedef void (*handler_fn_t)(struct ocpp_charger *charger,
		const struct ocpp_message *message);

struct handler_entry {
	ocpp_message_role_t role;
	ocpp_message_t type;
	handler_fn_t handler;
};

struct availability_ctx {
	bool unavailable;
	ocpp_availability_status_t status;
};

static void do_auth(struct ocpp_charger *charger,
		const struct ocpp_message *message)
{
	const struct ocpp_message *req = ocpp_get_message_by_id(message->id);
	struct ocpp_connector *oc = req? req->ctx : NULL;
	const struct ocpp_Authorize_conf *p =
		(const struct ocpp_Authorize_conf *)
		message->payload.fmt.response;

	if (!oc) {
		error("connector not found for message id %s", message->id);
	} else if (p->idTagInfo.status == OCPP_AUTH_STATUS_ACCEPTED) {
		if (ocpp_connector_is_session_established(oc)) { /* 2nd tag */
			if (ocpp_connector_is_trial_uid_equal(oc)) {
				ocpp_connector_trigger_stop_session(oc, false);
				info("session will stop by 2nd idTag");
			} else {
				error("session already established");
			}
			return;
		} else if (p->idTagInfo.expiryDate &&
				p->idTagInfo.expiryDate <= oc->now) {
			error("already expired");
			return;
		}
		ocpp_connector_accept_session_trial_uid(oc);
		ocpp_connector_set_session_pid(oc, p->idTagInfo.parentIdTag);

		uint32_t conn_timeout_sec;
		ocpp_get_configuration("ConnectionTimeOut",
				&conn_timeout_sec, sizeof(conn_timeout_sec), 0);
		time_t expiry = oc->now + conn_timeout_sec;
		if (p->idTagInfo.expiryDate) {
			expiry = MIN(expiry, p->idTagInfo.expiryDate);
		}
		ocpp_connector_set_session_current_expiry(oc, expiry);
	} else {
		ocpp_connector_clear_session_trial_uid(oc);
		error("session trial cleared: authorization failure");
	}
}

static void do_bootnoti(struct ocpp_charger *charger,
		const struct ocpp_message *message)
{
	const struct ocpp_BootNotification_conf *p =
		(const struct ocpp_BootNotification_conf *)
		message->payload.fmt.response;

	app_adjust_time_on_drift(p->currentTime, SYSTEM_TIME_MAX_DRIFT_SEC);
	ocpp_set_configuration("HeartbeatInterval",
			&p->interval, sizeof(p->interval));

	if (p->status == OCPP_BOOT_STATUS_ACCEPTED) {
		ocpp_charger_mq_send((struct charger *)charger,
				OCPP_CHARGER_MSG_CSMS_UP, NULL);
	} else {
		csms_request_defer(OCPP_MSG_BOOTNOTIFICATION,
				NULL, NULL, (uint32_t)p->interval);

		if (p->status == OCPP_BOOT_STATUS_REJECTED) {
			csms_reconnect((uint32_t)p->interval);
		}

		info("resend BootNotification after %d seconds",
				(uint32_t)p->interval);
	}
}

static void on_csl_chunk(const char *chunk, size_t chunk_len, void *ctx)
{
	int *measurands = (int *)ctx;
	ocpp_measurand_t t = ocpp_get_measurand_from_string(chunk, chunk_len);
	*measurands |= (int)t;
}

static int change_csl_configuration(const char *key, const char *value)
{
	const char *keys[] = {
		"MeterValuesAlignedData",
		"MeterValuesSampledData",
		"StopTxnAlignedData",
		"StopTxnSampledData",
	};
	const char *selected = NULL;

	for (size_t i = 0; i < ARRAY_COUNT(keys); i++) {
		if (strcmp(key, keys[i]) == 0) {
			selected = keys[i];
			break;
		}
	}

	if (selected == NULL) {
		return -ENOENT;
	}

	int measurands = 0;
	strchunk(value, ',', on_csl_chunk, &measurands);

	if (measurands) {
		ocpp_set_configuration(key, &measurands, sizeof(measurands));
	}

	return 0;
}

static void on_each_connector_availability(struct connector *c, void *ctx)
{
	struct ocpp_connector *oc = (struct ocpp_connector *)c;
	struct availability_ctx *p = (struct availability_ctx *)ctx;

	if (p->unavailable && (ocpp_connector_state(oc) == Charging ||
			ocpp_connector_state(oc) == SuspendedEV)) {
		p->status = OCPP_AVAILABILITY_STATUS_SCHEDULED;
	}

	ocpp_connector_set_availability(oc, !p->unavailable);
}

static void do_change_availability(struct ocpp_charger *charger,
		const struct ocpp_message *message)
{
	const struct ocpp_ChangeAvailability *p =
		(const struct ocpp_ChangeAvailability *)
		message->payload.fmt.request;
	struct charger *base = (struct charger *)charger;
	struct availability_ctx ctx = {
		.unavailable = (p->type == OCPP_INOPERATIVE),
		.status = OCPP_AVAILABILITY_STATUS_ACCEPTED,
	};
	const struct ocpp_checkpoint *checkpoint =
		ocpp_charger_get_checkpoint(base);

	if (p->connectorId == 0) { /* all connectors */
		charger_iterate_connectors(base,
				on_each_connector_availability, &ctx);

		ocpp_charger_set_availability(base, !ctx.unavailable);
	} else {
		struct ocpp_connector *c = (struct ocpp_connector *)
			charger_get_connector_by_id(base, p->connectorId);
		if (c) {
			ocpp_connector_set_availability(c, !ctx.unavailable);
		}
	}

	if (!ocpp_charger_is_checkpoint_equal(base, checkpoint)) {
		ocpp_charger_mq_send(base,
				OCPP_CHARGER_MSG_AVAILABILITY_CHANGED, NULL);
	}

	csms_response(OCPP_MSG_CHANGE_AVAILABILITY, message, NULL,
			(void *)ctx.status);
}

static bool is_configuration_reboot_required(const char *key)
{
	/* TODO: implement */
	return false;
}

static void do_change_configuration(struct ocpp_charger *charger,
		const struct ocpp_message *message)
{
	const struct ocpp_ChangeConfiguration *p =
		(const struct ocpp_ChangeConfiguration *)
		message->payload.fmt.request;
	const ocpp_configuration_data_t data_type =
		ocpp_get_configuration_data_type(p->key);
	ocpp_config_status_t status = OCPP_CONFIG_STATUS_ACCEPTED;
	int err = 0;

	if (data_type == OCPP_CONF_TYPE_UNKNOWN) {
		status = OCPP_CONFIG_STATUS_NOT_SUPPORTED;
	} else if (!ocpp_is_configuration_writable(p->key)) {
		status = OCPP_CONFIG_STATUS_REJECTED;
	} else if (data_type == OCPP_CONF_TYPE_STR) {
		err = ocpp_set_configuration(p->key,
				p->value, strlen(p->value)+1/*null*/);
	} else if (data_type == OCPP_CONF_TYPE_INT) {
		int val = (int)strtol(p->value, NULL, 10);
		err = ocpp_set_configuration(p->key, &val, sizeof(val));
	} else if (data_type == OCPP_CONF_TYPE_CSL) {
		err = change_csl_configuration(p->key, p->value);
	} else if (data_type == OCPP_CONF_TYPE_BOOL) {
		bool val = false;
		char str[8];
		strncpy(str, p->value, sizeof(str));
		strlower(str);
		if (strcmp(str, "true") == 0) {
			val = true;
		}
		err = ocpp_set_configuration(p->key, &val, sizeof(val));
	} else {
		status = OCPP_CONFIG_STATUS_NOT_SUPPORTED;
	}

	if (err) {
		status = OCPP_CONFIG_STATUS_REJECTED;
		error("failed to set configuration %s", p->key);
	} else {
		if (is_configuration_reboot_required(p->key)) {
			status = OCPP_CONFIG_STATUS_REBOOT_REQUIRED;
		}
		ocpp_charger_mq_send((struct charger *)charger,
				OCPP_CHARGER_MSG_CONFIGURATION_CHANGED, NULL);
	}

	csms_response(OCPP_MSG_CHANGE_CONFIGURATION,
			message, NULL, (void *)status);
}

static void do_clear_cache(struct ocpp_charger *charger,
		const struct ocpp_message *message)
{
	/* TODO: implement. always responses accepted as no cache implemented */
	csms_response(OCPP_MSG_CLEAR_CACHE, message, NULL,
			(void *)OCPP_REMOTE_STATUS_ACCEPTED);
}

static void do_get_configuration(struct ocpp_charger *charger,
		const struct ocpp_message *message)
{
	/* NOTE: An ad-hoc method to reduce memory usage, this handler does
	 * not process the request, but the encoder does. */
	csms_response(OCPP_MSG_GET_CONFIGURATION, message, NULL, NULL);
}

static void do_heartbeat(struct ocpp_charger *charger,
		const struct ocpp_message *message)
{
	const struct ocpp_Heartbeat_conf *p =
		(const struct ocpp_Heartbeat_conf *)
		message->payload.fmt.response;
	app_adjust_time_on_drift(p->currentTime, SYSTEM_TIME_MAX_DRIFT_SEC);
}

static void do_remote_start(struct ocpp_charger *charger,
		const struct ocpp_message *message)
{
	const struct ocpp_RemoteStartTransaction *p =
		(const struct ocpp_RemoteStartTransaction *)
		message->payload.fmt.request;
	struct connector *c;
	struct ocpp_connector *oc;

	if (p->connectorId == 0) {
		if ((c = charger_get_connector_available((struct charger *)
				charger)) == NULL) {
			error("no available connector");
			goto out;
		}
	}
	if ((c = charger_get_connector_by_id((struct charger *)charger,
			p->connectorId)) == NULL) {
		error("connector %d not found", p->connectorId);
		goto out;
	}

	oc = (struct ocpp_connector *)c;

	if (ocpp_connector_is_session_active(oc) ||
			ocpp_connector_is_session_trial_uid_exist(oc)) {
		error("connector %d is already occupied", p->connectorId);
		goto out;
	}

	ocpp_connector_state_t state = ocpp_connector_state(oc);

	if (state == Available || state == Preparing) {
		csms_response(OCPP_MSG_REMOTE_START_TRANSACTION, message, c,
				(void *)OCPP_REMOTE_STATUS_ACCEPTED);

		bool auth_required = false;
		ocpp_get_configuration("AuthorizeRemoteTxRequests",
				&auth_required, sizeof(auth_required), NULL);
		if (!auth_required) {
			ocpp_connector_set_session_current_uid(oc, p->idTag);
		} else {
			ocpp_connector_set_session_trial_uid(oc, p->idTag);
			csms_request(OCPP_MSG_AUTHORIZE, c, NULL);
		}
		return;
	}

out:
	csms_response(OCPP_MSG_REMOTE_START_TRANSACTION,
			message, c, (void *)OCPP_REMOTE_STATUS_REJECTED);
}

static void do_remote_stop(struct ocpp_charger *charger,
		const struct ocpp_message *message)
{
	const struct ocpp_RemoteStopTransaction *p =
		(const struct ocpp_RemoteStopTransaction *)
		message->payload.fmt.request;
	struct ocpp_connector *c = (struct ocpp_connector *)
		ocpp_charger_get_connector_by_tid((struct charger *)charger,
				(ocpp_transaction_id_t)p->transactionId);

	if (!c) {
		csms_response(OCPP_MSG_REMOTE_STOP_TRANSACTION, message, c,
				(void *)OCPP_REMOTE_STATUS_REJECTED);
		error("connector not found for transaction id %d",
				p->transactionId);
	} else {
		ocpp_connector_state_t state = ocpp_connector_state(c);
		if (state != Charging && state != SuspendedEV) {
			error("connector %d is not charging", c->base.id);
		}
		ocpp_connector_trigger_stop_session(c, true);
		csms_response(OCPP_MSG_REMOTE_STOP_TRANSACTION, message, c,
				(void *)OCPP_REMOTE_STATUS_ACCEPTED);
	}
}

static void do_reset(struct ocpp_charger *charger,
		const struct ocpp_message *message)
{
	const struct ocpp_Reset *p = (const struct ocpp_Reset *)
		message->payload.fmt.request;

	ocpp_charger_mq_send((struct charger *)charger,
			OCPP_CHARGER_MSG_REMOTE_RESET,
			(p->type == OCPP_RESET_HARD)?
				(void *)OCPP_CHARGER_REBOOT_FORCED :
				(void *)OCPP_CHARGER_REBOOT_REQUIRED_REMOTELY);

	csms_response(OCPP_MSG_RESET, message, NULL, charger);
}

static void do_start_transaction(struct ocpp_charger *charger,
		const struct ocpp_message *message)
{
	const struct ocpp_StartTransaction_conf *p =
		(const struct ocpp_StartTransaction_conf *)
		message->payload.fmt.response;
	const struct ocpp_message *req = ocpp_get_message_by_id(message->id);
	struct ocpp_connector *oc = req? req->ctx : NULL;

	if (!oc) {
		error("connector not found for message id %s", message->id);
		return;
	} else if (p->idTagInfo.status != OCPP_AUTH_STATUS_ACCEPTED) {
		ocpp_connector_clear_session(oc);
		error("session cleared: transaction start failure");
		return;
	}

	ocpp_connector_set_session_current_expiry(oc, p->idTagInfo.expiryDate);
	ocpp_connector_set_session_pid(oc, p->idTagInfo.parentIdTag);
	ocpp_connector_set_tid(oc, (ocpp_transaction_id_t)p->transactionId);

	ocpp_charger_mq_send((struct charger *)charger,
			OCPP_CHARGER_MSG_BILLING_STARTED, oc);
}

static void do_stop_transaction(struct ocpp_charger *charger,
		const struct ocpp_message *message)
{
	const struct ocpp_message *req = ocpp_get_message_by_id(message->id);
	struct ocpp_connector *oc = req? req->ctx : NULL;

	if (!oc) {
		error("connector not found for message id %s", message->id);
		return;
	}

	ocpp_charger_mq_send((struct charger *)charger,
			OCPP_CHARGER_MSG_BILLING_ENDED, oc);
}

static void do_updatefirmware(struct ocpp_charger *charger,
		const struct ocpp_message *message)
{
	const struct ocpp_UpdateFirmware *p =
		(const struct ocpp_UpdateFirmware *)
		message->payload.fmt.request;
	struct connector *c =
		charger_get_connector_by_id((struct charger *)charger, 1);

        /* NOTE: it will be dropped if already downloading, or overwritten if
         * pending. */
        struct updater_param param = {
		.retrieve_date = p->retrieveDate,
		.retry_interval = p->retryInterval,
		.max_retries = p->retries,
	};
	strcpy(param.url, p->url);

	/* TODO: support other protocols. https is only supported now */
	if (net_get_protocol_from_url(param.url) == NET_PROTO_HTTPS) {
		updater_request(&param, downloader_http_create());
	} else {
		error("unsupported protocol in URL: %s", param.url);
	}

	csms_response(OCPP_MSG_UPDATE_FIRMWARE, message, c, NULL);
}

static struct handler_entry handlers[] = {
	{ OCPP_MSG_ROLE_CALLRESULT, OCPP_MSG_AUTHORIZE,                do_auth },
	{ OCPP_MSG_ROLE_CALLRESULT, OCPP_MSG_BOOTNOTIFICATION,         do_bootnoti },
	{ OCPP_MSG_ROLE_CALL,       OCPP_MSG_CHANGE_AVAILABILITY,      do_change_availability },
	{ OCPP_MSG_ROLE_CALL,       OCPP_MSG_CHANGE_CONFIGURATION,     do_change_configuration },
	{ OCPP_MSG_ROLE_CALL,       OCPP_MSG_CLEAR_CACHE,              do_clear_cache },
	{ OCPP_MSG_ROLE_CALL,       OCPP_MSG_GET_CONFIGURATION,        do_get_configuration },
	{ OCPP_MSG_ROLE_CALLRESULT, OCPP_MSG_HEARTBEAT,                do_heartbeat },
	{ OCPP_MSG_ROLE_CALL,       OCPP_MSG_REMOTE_START_TRANSACTION, do_remote_start },
	{ OCPP_MSG_ROLE_CALL,       OCPP_MSG_REMOTE_STOP_TRANSACTION,  do_remote_stop },
	{ OCPP_MSG_ROLE_CALL,       OCPP_MSG_RESET,                    do_reset },
	{ OCPP_MSG_ROLE_CALLRESULT, OCPP_MSG_START_TRANSACTION,        do_start_transaction },
	{ OCPP_MSG_ROLE_CALLRESULT, OCPP_MSG_STOP_TRANSACTION,         do_stop_transaction },
	{ OCPP_MSG_ROLE_CALL,       OCPP_MSG_UPDATE_FIRMWARE,          do_updatefirmware },
};

void handler_process(struct ocpp_charger *charger,
		const struct ocpp_message *message)
{
	for (size_t i = 0; i < ARRAY_COUNT(handlers); i++) {
		if (handlers[i].role == message->role &&
				handlers[i].type == message->type) {
			handlers[i].handler(charger, message);
			return;
		}
	}

	warn("no handler for message type %s in role %d",
			ocpp_stringify_type(message->type), message->role);
}
