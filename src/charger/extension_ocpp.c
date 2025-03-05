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

#include "charger/ocpp.h"
#include "ocpp/ocpp_connector_internal.h"
#include "charger_internal.h"

#include <string.h>

#include "ocpp/csms.h"
#include "updater.h"
#include "config.h"
#include "libmcu/compiler.h"

#if !defined(ARRAY_SIZE)
#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))
#endif

typedef void (*msg_handler_t)(struct charger *charger,
		const struct ocpp_charger_msg *msg);

static void on_updater_event(updater_event_t event, void *ctx)
{
	const ocpp_comm_status_t evtbl[] = {
		[UPDATER_EVT_NONE]              = OCPP_COMM_IDLE,
		[UPDATER_EVT_ERROR_PREPARING]   = OCPP_COMM_DOWNLOAD_FAILED,
		[UPDATER_EVT_ERROR_DOWNLOADING] = OCPP_COMM_DOWNLOAD_FAILED,
		[UPDATER_EVT_ERROR_WRITING]     = OCPP_COMM_DOWNLOAD_FAILED,
		[UPDATER_EVT_START_DOWNLOADING] = OCPP_COMM_DOWNLOADING,
		[UPDATER_EVT_DOWNLOADING]       = OCPP_COMM_IDLE,
		[UPDATER_EVT_DOWNLOADED]        = OCPP_COMM_DOWNLOADED,
		[UPDATER_EVT_START_INSTALLING]  = OCPP_COMM_INSTALLING,
		[UPDATER_EVT_ERROR_INSTALLING]  = OCPP_COMM_INSTALLATION_FAILED,
		[UPDATER_EVT_REBOOT_REQUIRED]   = OCPP_COMM_IDLE,
		[UPDATER_EVT_INSTALLED]         = OCPP_COMM_INSTALLED,
		[UPDATER_EVT_ERROR_TIMEOUT]     = OCPP_COMM_IDLE,
		[UPDATER_EVT_ERROR]             = OCPP_COMM_IDLE,
	};
	ocpp_comm_status_t status = evtbl[event];
	struct charger *charger = (struct charger *)ctx;

	if (status == OCPP_COMM_IDLE) {
		if (event == UPDATER_EVT_REBOOT_REQUIRED) {
			ocpp_charger_request_reboot(charger,
					OCPP_CHARGER_REBOOT_REQUIRED);
		}
		return;
	}

	csms_request(OCPP_MSG_FIRMWARE_NOTIFICATION, NULL, (void *)status);
}

static void on_each_connector_csms_up(struct connector *c, void *ctx)
{
	struct ocpp_connector *oc = (struct ocpp_connector *)c;
	ocpp_connector_set_csms_up(oc, true);
}

static void on_each_connector_reboot(struct connector *c, void *ctx)
{
	struct ocpp_connector *oc = (struct ocpp_connector *)c;
	ocpp_charger_reboot_t *type = (ocpp_charger_reboot_t *)ctx;
	ocpp_connector_set_remote_reset_requested(oc, true,
			*type == OCPP_CHARGER_REBOOT_FORCED);
}

static void dispatch_event(struct charger *charger, struct connector *c,
		const charger_event_t event)
{
	if (!event || !charger->event_cb) {
		return;
	}

	(*charger->event_cb)(charger, c, event, charger->event_cb_ctx);
}

static void proc_availability(struct charger *charger,
		const struct ocpp_charger_msg *msg)
{
	struct ocpp_connector_checkpoint checkpoint = {
		.unavailable = !ocpp_charger_get_availability(charger),
	};
	struct ocpp_connector c0 = {
		.checkpoint = &checkpoint,
		.now = time(NULL),
	};
	csms_request(OCPP_MSG_STATUS_NOTIFICATION, &c0, CONNECTOR_0);

	dispatch_event(charger, NULL, OCPP_CHARGER_EVENT_AVAILABILITY_CHANGED);
}

static void proc_configuration(struct charger *charger,
		const struct ocpp_charger_msg *msg)
{
	unused(msg);
	dispatch_event(charger, NULL, OCPP_CHARGER_EVENT_CONFIGURATION_CHANGED);
}

static void proc_csms_up(struct charger *charger,
		const struct ocpp_charger_msg *msg)
{
	struct ocpp_connector_checkpoint checkpoint = {
		.unavailable = (bool)(uintptr_t)msg->value,
	};
	struct ocpp_connector c0 = {
		.checkpoint = &checkpoint,
		.now = time(NULL),
	};
	charger_iterate_connectors(charger, on_each_connector_csms_up, NULL);
	csms_request(OCPP_MSG_STATUS_NOTIFICATION, &c0, CONNECTOR_0);
}

static void proc_remote_reset(struct charger *charger,
		const struct ocpp_charger_msg *msg)
{
	ocpp_charger_reboot_t type = (ocpp_charger_reboot_t)
		(uintptr_t)msg->value;
	ocpp_charger_request_reboot(charger, type);
	charger_iterate_connectors(charger, on_each_connector_reboot, &type);
}

static void proc_start_transaction(struct charger *charger,
		const struct ocpp_charger_msg *msg)
{
	struct ocpp_connector *oc = (struct ocpp_connector *)msg->value;
	if (oc == NULL) {
		error("no connector given for StartTransaction");
		return;
	}
	ocpp_connector_raise_event(oc, CONNECTOR_EVENT_BILLING_STARTED);
}

static void proc_stop_transaction(struct charger *charger,
		const struct ocpp_charger_msg *msg)
{
	struct ocpp_connector *oc = (struct ocpp_connector *)msg->value;
	if (oc == NULL) {
		error("no connector given for StopTransaction");
		return;
	}
	ocpp_connector_clear_checkpoint_tid(oc);
	ocpp_connector_raise_event(oc, CONNECTOR_EVENT_BILLING_ENDED);
}

static int ext_init(struct charger *self)
{
	struct ocpp_charger *charger = (struct ocpp_charger *)self;
	struct ocpp_checkpoint checkpoint = { 0, };

	config_read(CONFIG_KEY_OCPP_CHECKPOINT,
			&checkpoint, sizeof(checkpoint));
	ocpp_charger_set_checkpoint(self, &checkpoint);

	int err = csms_init(self);

	if (!err) {
		err = updater_register_event_callback(on_updater_event, self);
	}

	return err;
}

static void ext_deinit(struct charger *self)
{
	unused(self);
}

static int ext_pre_process(struct charger *self)
{
	const msg_handler_t msg_handlers[] = {
		[OCPP_CHARGER_MSG_AVAILABILITY_CHANGED] = proc_availability,
		[OCPP_CHARGER_MSG_CONFIGURATION_CHANGED] = proc_configuration,
		[OCPP_CHARGER_MSG_BILLING_STARTED] = proc_start_transaction,
		[OCPP_CHARGER_MSG_BILLING_ENDED] = proc_stop_transaction,
		[OCPP_CHARGER_MSG_CSMS_UP] = proc_csms_up,
		[OCPP_CHARGER_MSG_REMOTE_RESET] = proc_remote_reset,
	};
	static_assert(ARRAY_SIZE(msg_handlers) == OCPP_CHARGER_MSG_MAX,
			"msg_handlers size mismatch");

	struct ocpp_charger_msg msg = { 0, };

	int err = ocpp_step();

	if (ocpp_charger_mq_recv(self, &msg) >= 0 &&
			msg.type < OCPP_CHARGER_MSG_MAX) {
		(*msg_handlers[msg.type])(self, &msg);
	}

	return err;
}

static int ext_post_process(struct charger *self)
{
	unused(self);
	return 0;
}

struct charger_extension *ocpp_charger_extension(void)
{
	static struct charger_extension ocpp_ext = {
		.init = ext_init,
		.deinit = ext_deinit,
		.pre_process = ext_pre_process,
		.post_process = ext_post_process,
	};

	return &ocpp_ext;
}
