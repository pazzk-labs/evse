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

#include "connector_private.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "csms.h"
#include "updater.h"
#include "libmcu/metrics.h"
#include "logger.h"

#define DEFAULT_PROCESSING_INTERVAL_MS		10

static void dispatch_event(struct charger *charger, struct connector *c,
		const charger_event_t event)
{
	if (!event || !charger->event_cb) {
		return;
	}

	(*charger->event_cb)(charger, c, event, charger->event_cb_ctx);
}

static void process_event(struct ocpp_charger *charger, const bool charging)
{
	struct charger *base = &charger->base;
	charger_event_t event = CHARGER_EVENT_NONE;

	if (charger->configuration_changed) {
		charger->configuration_changed = false;
		event |= CHARGER_EVENT_CONFIGURATION_CHANGED;
	}

	if (charger->param_changed) {
		charger->param_changed = false;
		event |= CHARGER_EVENT_PARAM_CHANGED;
	}

	if (charger->reboot_required && !charging &&
			ocpp_count_pending_requests() == 0) {
		charger->reboot_required = OCPP_CHARGER_REBOOT_NONE;
		event |= CHARGER_EVENT_REBOOT_REQUIRED;
	}

	if (event) {
		dispatch_event(base, NULL, event);
	}
}

static void on_state_change(struct fsm *fsm,
		fsm_state_t new_state, fsm_state_t prev_state, void *ctx)
{
	struct ocpp_connector *c = (struct ocpp_connector *)ctx;
	c->base.time_last_state_change = c->now;

	info("connector(%d) state changed: %s -> %s at %ld", c->base.id,
			stringify_status(prev_state),
			stringify_status(new_state),
			c->base.time_last_state_change);

	const connector_state_t cp_new = (connector_state_t)s2cp(new_state);
	const connector_state_t cp_prev = (connector_state_t)s2cp(prev_state);

	if (cp_new != cp_prev) {
		const charger_event_t events =
			get_event_from_state_change(cp_new, cp_prev);
		dispatch_event(c->base.charger, NULL, events);
	}
}

static void on_updater_event(updater_event_t event, void *ctx)
{
	const ocpp_comm_status_t evtbl[] = {
		[UPDATER_EVT_NONE] = OCPP_COMM_IDLE,
		[UPDATER_EVT_ERROR_PREPARING] = OCPP_COMM_DOWNLOAD_FAILED,
		[UPDATER_EVT_ERROR_DOWNLOADING] = OCPP_COMM_DOWNLOAD_FAILED,
		[UPDATER_EVT_ERROR_WRITING] = OCPP_COMM_DOWNLOAD_FAILED,
		[UPDATER_EVT_START_DOWNLOADING] = OCPP_COMM_DOWNLOADING,
		[UPDATER_EVT_DOWNLOADING] = OCPP_COMM_IDLE,
		[UPDATER_EVT_DOWNLOADED] = OCPP_COMM_DOWNLOADED,
		[UPDATER_EVT_START_INSTALLING] = OCPP_COMM_INSTALLING,
		[UPDATER_EVT_ERROR_INSTALLING] = OCPP_COMM_INSTALLATION_FAILED,
		[UPDATER_EVT_REBOOT_REQUIRED] = OCPP_COMM_IDLE,
		[UPDATER_EVT_INSTALLED] = OCPP_COMM_INSTALLED,
		[UPDATER_EVT_ERROR_TIMEOUT] = OCPP_COMM_IDLE,
		[UPDATER_EVT_ERROR] = OCPP_COMM_IDLE,
	};
	ocpp_comm_status_t status = evtbl[event];
	struct ocpp_charger *charger = (struct ocpp_charger *)ctx;
	struct connector *c = charger_get_connector_by_id(&charger->base, 1);

	if (status == OCPP_COMM_IDLE) {
		if (event == UPDATER_EVT_REBOOT_REQUIRED &&
				charger->reboot_required
				== OCPP_CHARGER_REBOOT_NONE) {
			charger->reboot_required = OCPP_CHARGER_REBOOT_REQUIRED;
		}
		return;
	}

	csms_request(OCPP_MSG_FIRMWARE_NOTIFICATION, c, (void *)status);
}

static int step(struct charger *self, uint32_t *next_period_ms)
{
	struct ocpp_charger *charger = (struct ocpp_charger *)self;
	time_t now = time(NULL);
	bool charging = false;
	struct list *p;

	ocpp_step();

	list_for_each(p, &self->connectors.list) {
		struct ocpp_connector *c = list_entry(p,
				struct ocpp_connector, base.link);
		c->now = now;
		fsm_step(&c->fsm);

		ocpp_connector_status_t status = get_status(c);
		if (status == Charging || status == SuspendedEV) {
			int err = metering_step(c->base.param.metering);
			if (err != 0 && err != -EAGAIN) {
				ratelim_request_format(&self->log_ratelim,
					logger_error, "metering_step failed");
			}
			charging = true;
		}
	}

	process_event(charger, charging);

	if (next_period_ms) {
		*next_period_ms = DEFAULT_PROCESSING_INTERVAL_MS;
	}

	return 0;
}

static struct connector *create_connector(struct charger *charger,
		const struct connector_param *param)
{
	struct ocpp_connector *c;
	struct ocpp_charger *ocpp_charger = (struct ocpp_charger *)charger;

	if (charger->connectors.count >= OCPP_CHARGER_MAX_CONNECTORS ||
			metering_enable(param->metering) != 0 ||
			!(c = (struct ocpp_connector *)malloc(sizeof(*c)))) {
		return NULL;
	}

	memset(c, 0, sizeof(*c));

	register_connector(&c->base, param, charger);

	size_t n = 0;
	const struct fsm_item *transitions = get_ocpp_fsm_table(&n);
	fsm_init(&c->fsm, transitions, n, c);
	fsm_set_state_change_cb(&c->fsm, on_state_change, c);

	c->unavailable = &ocpp_charger->param.unavailability
		.connector[charger->connectors.count];

	return &c->base;
}

static void destroy_connector(struct charger *charger, struct list *link)
{
	struct ocpp_connector *c =
		list_entry(link, struct ocpp_connector, base.link);

	if (c) {
		metering_disable(c->base.param.metering);
		list_del(&c->base.link, &charger->connectors.list);
		free(c);
	}
}

static const struct charger_api *get_api(void)
{
	static struct charger_api api = {
		.create_connector = create_connector,
		.destroy_connector = destroy_connector,
		.step = step,
	};

	return &api;
}

void ocpp_charger_backup_tid(struct connector *connector)
{
	struct ocpp_connector *c = (struct ocpp_connector *)connector;
	struct ocpp_charger *charger = (struct ocpp_charger *)c->base.charger;
	charger->param.missing_transaction_id = c->session.transaction_id;
}

void ocpp_charger_clear_backup_tid(struct connector *connector)
{
	struct ocpp_connector *c = (struct ocpp_connector *)connector;
	struct ocpp_charger *charger = (struct ocpp_charger *)c->base.charger;
	charger->param.missing_transaction_id = 0;
}

const struct ocpp_charger_param *ocpp_charger_get_param(struct charger *charger)
{
	return &((struct ocpp_charger *)charger)->param;
}

struct charger *ocpp_charger_create(const struct charger_param *param,
		const struct ocpp_charger_param *ocpp_param)
{
	static struct ocpp_charger ocpp_charger;
	struct charger *charger = (struct charger *)&ocpp_charger;

	memset(&ocpp_charger, 0, sizeof(ocpp_charger));

	*charger = (struct charger) {
		.api = *get_api(),
		.name = "ocpp",
	};

	if (ocpp_param) {
		memcpy(&ocpp_charger.param, ocpp_param, sizeof(*ocpp_param));
	}

	if (charger_init(charger, param) == 0 && csms_init(charger) == 0) {
		updater_register_event_callback(on_updater_event, charger);
		return charger;
	}

	return NULL;
}

void ocpp_charger_destroy(struct charger *charger)
{
	if (!charger) {
		return;
	}

	charger_deinit(charger);
}
