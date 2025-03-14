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

#include "ocpp_connector_internal.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "csms.h"
#include "metering.h"
#include "logger.h"

#define MAX_EVENTS			4

#if !defined(ARRAY_COUNT)
#define ARRAY_COUNT(x)			(sizeof(x) / sizeof((x)[0]))
#endif

static bool is_booting(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);
	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;
	return !ocpp_connector_is_csms_up(oc) &&
		ocpp_count_pending_requests() == 0;
}

static bool is_available(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);

	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;
	struct connector *c = &oc->base;

	if (!ocpp_connector_is_csms_up(oc)) { /* boot first */
		return false;
	}

	if (!connector_is_state_x(c, A)) {
		return false;
	}
	if (!ocpp_connector_is_session_active(oc)) { /* 2nd idTag or timeout */
		info("session %s", ocpp_connector_is_session_established(oc)?
				"expired" : "not active");
		return true;
	}

	return false;
}

static bool is_preparing(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);

	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;
	struct connector *c = &oc->base;

	if (!ocpp_connector_is_csms_up(oc)) { /* boot first */
		return false;
	}

	return connector_is_state_x(c, B) || (connector_is_state_x(c, A) &&
			ocpp_connector_is_session_active(oc));
}

static bool is_charging(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(next_state);

	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;
	struct connector *c = &oc->base;

	if (state == Preparing) {
		return connector_is_state_x(c, B) &&
			ocpp_connector_is_session_established(oc) &&
			ocpp_connector_is_transaction_started(oc);
	} else if (state == SuspendedEV) {
		return connector_is_state_x2(c, C) ||
			connector_is_state_x2(c, D);
	}

	return connector_is_state_x2(c, C) || connector_is_state_x2(c, D) ||
		(ocpp_connector_is_session_established(oc) &&
			ocpp_connector_is_transaction_started(oc));
}

static bool is_charging_rdy(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(next_state);

	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;
	struct connector *c = &oc->base;

	if (state != Preparing) {
		return false;
	}

	return connector_is_state_x(c, B) &&
		ocpp_connector_is_session_active(oc) &&
		ocpp_count_pending_requests() == 0;
}

static bool is_suspendedEV(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(next_state);

	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;
	struct connector *c = &oc->base;

	if (state != Charging && state != SuspendedEV) {
		return false;
	}
	return connector_is_state_x(c, B);
}

static bool is_finishing(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(next_state);

	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;
	struct connector *c = &oc->base;

	if (state != SuspendedEV && state != Charging) {
		return false;
	}

	if (ocpp_connector_is_remote_reset_requested(oc, NULL) ||
			connector_is_state_x(c, A) ||
			connector_is_state_x(c, F) ||
			!ocpp_connector_is_session_established(oc)/*2nd tag*/) {
		return true;
	}

	return false;
}

static bool is_reserved(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);
	unused(ctx);
	/* TODO: implement */
	return false;
}

static bool is_unavailable(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);

	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;
	struct connector *c = &oc->base;

	if (!ocpp_connector_is_csms_up(oc)) { /* boot first */
		return false;
	}

	if (connector_is_state_x(c, F)) {
		return false;
	}

	return ocpp_connector_is_unavailable(oc);
}

static bool is_faulted(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(next_state);

	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;
	struct connector *c = &oc->base;
	const ocpp_connector_state_t ocpp_state = (ocpp_connector_state_t)state;

	if (connector_is_evse_error(c,
			ocpp_connector_map_state_to_common(ocpp_state))) {
		return true;
	}

	return false;
}

static void send_connector_status(struct ocpp_connector *oc, void *opt)
{
	csms_request(OCPP_MSG_STATUS_NOTIFICATION, oc, opt);
}

static void send_metering(struct ocpp_connector *oc)
{
	ocpp_measurand_t flags;

	if ((flags = ocpp_connector_update_metering(oc))) {
		csms_request(OCPP_MSG_METER_VALUES, oc, (void *)flags);
	}

	if ((flags = ocpp_connector_update_metering_clock_aligned(oc))) {
		csms_request(OCPP_MSG_METER_VALUES, oc, (void *)flags);
	}
}

static void do_boot(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);

	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;
	struct connector *c = &oc->base;
	connector_stop_duty(c);
	csms_request(OCPP_MSG_BOOTNOTIFICATION, oc, 0);
}

static void do_missing_transaction(fsm_state_t state, fsm_state_t next_state,
		void *ctx)
{
	unused(next_state);

	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;
	struct connector *c = &oc->base;
	const ocpp_connector_state_t ocpp_state = (ocpp_connector_state_t)state;

	if (ocpp_state != Booting) {
		return;
	}

	if (ocpp_connector_has_missing_transaction(oc)) {
		const ocpp_stop_reason_t reason =
			ocpp_connector_get_stop_reason(oc, ocpp_state);
		/* Sets the missing transaction id for the charging session and
		 * sends the current metering value. This arbitrarily set
		 * charging session information cleared afterward. */
		ocpp_connector_set_tid(oc, oc->checkpoint->transaction_id);
		metering_get_energy(connector_meter(c),
				&oc->session.metering.stop_wh, NULL);
		csms_request(OCPP_MSG_STOP_TRANSACTION, oc, (void *)reason);
		ocpp_connector_clear_session(oc);
	}
}

static void do_available(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;

	if (state == Preparing && !ocpp_connector_is_session_established(oc)) {
		ocpp_connector_raise_event(oc, CONNECTOR_EVENT_UNPLUGGED);
	}

	send_connector_status(oc, NULL);
	ocpp_connector_clear_session(oc);

	do_missing_transaction(state, next_state, ctx);
}

static void do_preparing(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;

        /* NOTE: Transition to the Preparing state to allow the user to restart
         * charging if desired, but maintain the Finishing state in the CSMS to
         * indicate that the user has not removed the connector after completing
         * the charge. */
	if (state != Finishing) {
		csms_request(OCPP_MSG_STATUS_NOTIFICATION, oc, 0);
	}

	do_missing_transaction(state, next_state, ctx);
}

static void do_request_tid(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);

	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;
	struct connector *c = &oc->base;

	metering_get_energy(connector_meter(c),
			&oc->session.metering.start_wh, NULL);
	csms_request(OCPP_MSG_START_TRANSACTION, oc, 0);

	/* NOTE: This ensures the id is not maintained after unplugging. */
	ocpp_connector_set_session_current_expiry(oc, oc->now);
}

static void do_charging(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(next_state);

	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;
	struct connector *c = &oc->base;

	if (state == Charging) {
		if (!connector_is_supplying_power(c)) {
			connector_enable_power_supply(c);
			send_connector_status(oc, NULL);
		}
	} else {
		if (state == Preparing) {
			connector_start_duty(c);
			return;
		}
	}

	send_metering(oc);
}

static void do_suspendedEV(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(next_state);

	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;
	struct connector *c = &oc->base;

	if (state == Charging) {
		connector_disable_power_supply(c);
	}

	send_connector_status(oc, NULL);
}

static void do_finishing(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(next_state);

	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;
	struct connector *c = &oc->base;
	const ocpp_connector_state_t ocpp_state = (ocpp_connector_state_t)state;

	if (ocpp_state == Charging || ocpp_state == SuspendedEV) {
		const ocpp_stop_reason_t reason =
			ocpp_connector_get_stop_reason(oc, ocpp_state);
		if (ocpp_state == Charging) {
			connector_disable_power_supply(c);
		}
		metering_get_energy(connector_meter(c),
				&oc->session.metering.stop_wh, NULL);
		csms_request(OCPP_MSG_STOP_TRANSACTION, oc, (void *)reason);
	}

	connector_stop_duty(c);

	send_connector_status(oc, NULL);
	ocpp_connector_clear_session(oc);
}

static void do_reserved(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);
	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;
	send_connector_status(oc, NULL);
}

static void do_unavailable(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;

	send_connector_status(oc, NULL);
	ocpp_connector_clear_session(oc);

	do_missing_transaction(state, next_state, ctx);
}

static void do_faulted(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	unused(state);
	unused(next_state);

	struct ocpp_connector *oc = (struct ocpp_connector *)ctx;
	struct connector *c = &oc->base;
	union {
		void *opt;
		const char *str;
	} info = { .opt = NULL };

	if (connector_is_emergency_stop(c)) {
		info.str = "EmergencyStop";
	}

	send_connector_status(oc, info.opt);
	ocpp_connector_clear_session(oc);
}

static const struct fsm_item transitions[] = {
	FSM_ITEM(Booting,     is_booting,       do_boot,        Booting),
	FSM_ITEM(Booting,     is_unavailable,   do_unavailable, Unavailable),
	FSM_ITEM(Booting,     is_available,     do_available,   Available),
	FSM_ITEM(Booting,     is_preparing,     do_preparing,   Preparing),
	FSM_ITEM(Available,   is_faulted,       do_faulted,     Faulted),
	FSM_ITEM(Available,   is_unavailable,   do_unavailable, Unavailable),
	FSM_ITEM(Available,   is_preparing,     do_preparing,   Preparing),
	FSM_ITEM(Available,   is_reserved,      do_reserved,    Reserved),
	FSM_ITEM(Preparing,   is_faulted,       do_faulted,     Faulted),
	FSM_ITEM(Preparing,   is_unavailable,   do_unavailable, Unavailable),
	FSM_ITEM(Preparing,   is_available,     do_available,   Available),
	FSM_ITEM(Preparing,   is_charging_rdy,  do_request_tid, Preparing),
	FSM_ITEM(Preparing,   is_charging,      do_charging,    Charging),
	FSM_ITEM(Preparing,   is_finishing,     do_finishing,   Finishing),
	FSM_ITEM(Charging,    is_suspendedEV,   do_suspendedEV, SuspendedEV),
	FSM_ITEM(Charging,    is_finishing,     do_finishing,   Finishing),
	FSM_ITEM(Charging,    is_charging,      do_charging,    Charging),
	FSM_ITEM(SuspendedEV, is_charging,      do_charging,    Charging),
	FSM_ITEM(SuspendedEV, is_finishing,     do_finishing,   Finishing),
	FSM_ITEM(SuspendedEV, is_suspendedEV,   do_charging,    SuspendedEV),
	FSM_ITEM(Finishing,   is_faulted,       do_faulted,     Faulted),
	FSM_ITEM(Finishing,   is_unavailable,   do_unavailable, Unavailable),
	FSM_ITEM(Finishing,   is_available,     do_available,   Available),
	FSM_ITEM(Finishing,   is_preparing,     do_preparing,   Preparing),
	FSM_ITEM(Reserved,    is_faulted,       do_faulted,     Faulted),
	FSM_ITEM(Reserved,    is_unavailable,   do_unavailable, Unavailable),
	FSM_ITEM(Reserved,    is_available,     do_available,   Available),
	FSM_ITEM(Reserved,    is_preparing,     do_preparing,   Preparing),
	FSM_ITEM(Unavailable, is_unavailable,   NULL,           Unavailable),
	FSM_ITEM(Unavailable, is_faulted,       do_faulted,     Faulted),
	FSM_ITEM(Unavailable, is_available,     do_available,   Available),
	FSM_ITEM(Unavailable, is_preparing,     do_preparing,   Preparing),
	FSM_ITEM(Faulted,     is_faulted,       NULL,           Faulted),
	FSM_ITEM(Faulted,     is_unavailable,   do_unavailable, Unavailable),
	FSM_ITEM(Faulted,     is_available,     do_available,   Available),
	FSM_ITEM(Faulted,     is_preparing,     do_preparing,   Preparing),
};

static connector_event_t get_event_from_state_change(fsm_state_t new_state,
		fsm_state_t prev_state, bool plugged)
{
	const struct {
		const fsm_state_t from;
		const fsm_state_t to;
		const bool plugged;
		const uint32_t event;
	} tbl[] = {
		{ Available,   Preparing,   true,  CONNECTOR_EVENT_PLUGGED |
			CONNECTOR_EVENT_OCCUPIED },
		{ Available,   Preparing,   false, CONNECTOR_EVENT_OCCUPIED },
		{ Available,   Reserved,    false, CONNECTOR_EVENT_RESERVED },
		{ Available,   Faulted,     false, CONNECTOR_EVENT_ERROR },
		{ Preparing,   Available,   false, CONNECTOR_EVENT_UNOCCUPIED },
		{ Preparing,   Charging,    true,  CONNECTOR_EVENT_CHARGING_STARTED },
		{ Preparing,   SuspendedEV, true,  CONNECTOR_EVENT_CHARGING_SUSPENDED },
		{ Preparing,   Faulted,     true,  CONNECTOR_EVENT_ERROR },
		{ Charging,    Finishing,   false, CONNECTOR_EVENT_CHARGING_ENDED },
		{ Charging,    Finishing,   true,  CONNECTOR_EVENT_CHARGING_ENDED },
		{ Charging,    SuspendedEV, true,  CONNECTOR_EVENT_CHARGING_SUSPENDED },
		{ SuspendedEV, Charging,    true,  CONNECTOR_EVENT_CHARGING_STARTED },
		{ SuspendedEV, Finishing,   true,  CONNECTOR_EVENT_CHARGING_ENDED },
		{ SuspendedEV, Finishing,   false, CONNECTOR_EVENT_CHARGING_ENDED },
		{ Finishing,   Available,   false, CONNECTOR_EVENT_UNPLUGGED |
			CONNECTOR_EVENT_UNOCCUPIED },
		{ Finishing,   Faulted,     false, CONNECTOR_EVENT_ERROR |
			CONNECTOR_EVENT_UNPLUGGED | CONNECTOR_EVENT_UNOCCUPIED },
		{ Finishing,   Faulted,     true,  CONNECTOR_EVENT_ERROR },
		{ Reserved,    Available,   false, CONNECTOR_EVENT_UNOCCUPIED },
		{ Reserved,    Preparing,   true,  CONNECTOR_EVENT_PLUGGED },
		{ Faulted,     Available,   false, CONNECTOR_EVENT_ERROR_RECOVERY },
		{ Faulted,     Preparing,   true,  CONNECTOR_EVENT_ERROR_RECOVERY },
	};

	for (size_t i = 0; i < ARRAY_COUNT(tbl); i++) {
		if (tbl[i].from == prev_state && tbl[i].to == new_state &&
				tbl[i].plugged == plugged) {
			return (connector_event_t)tbl[i].event;
		}
	}

	return CONNECTOR_EVENT_NONE;
}

static void dispatch_event(struct connector *c, connector_event_t event)
{
	struct ocpp_connector *oc = (struct ocpp_connector *)c;
	uint32_t events = event;

	while (msgq_len(oc->evtq) > 0) {
		connector_event_t evt;
		if (msgq_pop(oc->evtq, &evt, sizeof(evt)) > 0) {
			events |= evt;
		}
	}

	if (events && c->event_cb) {
		(*c->event_cb)(c, (connector_event_t)events, c->event_cb_ctx);
	}
}

static void on_state_change(struct fsm *fsm,
		fsm_state_t new_state, fsm_state_t prev_state, void *ctx)
{
	unused(fsm);

	struct connector *c = (struct connector *)ctx;
	const connector_state_t cp_state = connector_state(c);

	c->time_last_state_change = time(NULL);

	info("connector \"%s\" state changed: %s to %s at %ld",
			connector_name(c),
			ocpp_connector_stringify_state(
					(ocpp_connector_state_t)prev_state),
			ocpp_connector_stringify_state(
					(ocpp_connector_state_t)new_state),
			c->time_last_state_change);

	dispatch_event(c, get_event_from_state_change(new_state, prev_state,
			connector_is_occupied_state(cp_state)));

	connector_update_metrics(cp_state);
}

static int process(struct connector *self)
{
	struct ocpp_connector *oc = (struct ocpp_connector *)self;

	oc->now = time(NULL);

	const fsm_state_t state = fsm_step(&self->fsm);
	struct metering *meter = connector_meter(self);

	if (meter && (state == Charging || state == SuspendedEV)) {
		int err = metering_step(meter);
		if (err && err != -EAGAIN) {
			ratelim_request_format(&self->log_ratelim,
				logger_error, "metering_step failed");
		}
	}

	dispatch_event(self, CONNECTOR_EVENT_NONE);

	return 0;
}

static int enable(struct connector *self)
{
	(void)self;
	return 0;
}

static int disable(struct connector *self)
{
	(void)self;
	return 0;
}

static const struct connector_api *get_api(void)
{
	static struct connector_api api = {
		.enable = enable,
		.disable = disable,
		.process = process,
	};

	return &api;
}

int ocpp_connector_link_checkpoint(struct connector *c,
		struct ocpp_connector_checkpoint *checkpoint)
{
	struct ocpp_connector *oc = (struct ocpp_connector *)c;
	oc->checkpoint = checkpoint;
	return 0;
}

struct connector *ocpp_connector_create(const struct connector_param *param)
{
	struct ocpp_connector *oc;

	if (!param || !connector_validate_param(param) ||
			(oc = (struct ocpp_connector *)malloc(sizeof(*oc)))
				== NULL) {
		return NULL;
	}

	*oc = (struct ocpp_connector) {
		.base = {
			.api = get_api(),
			.param = *param,
		},
	};

	if ((oc->evtq = msgq_create(msgq_calc_size(MAX_EVENTS,
			sizeof(connector_event_t)))) == NULL) {
		free(oc);
		return NULL;
	}

	fsm_init(&oc->base.fsm, transitions, ARRAY_COUNT(transitions), oc);
	fsm_set_state_change_cb(&oc->base.fsm, on_state_change, oc);

	ratelim_init(&oc->base.log_ratelim, RATELIM_UNIT_SECOND,
			CONNECTOR_LOG_RATE_CAP, CONNECTOR_LOG_RATE_SEC);

	info("connector \"%s\" created", param->name);

	return &oc->base;
}

void ocpp_connector_destroy(struct connector *c)
{
	struct ocpp_connector *oc = (struct ocpp_connector *)c;
	msgq_destroy(oc->evtq);
	free(oc);
}
