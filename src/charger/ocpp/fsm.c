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

#include "csms.h"
#include "logger.h"

#if !defined(ARRAY_SIZE)
#define ARRAY_SIZE(x)			(sizeof(x) / sizeof((x)[0]))
#endif

static bool is_booting(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *c = (struct ocpp_connector *)ctx;
	return !is_csms_up(c) && ocpp_count_pending_requests() == 0;
}

static bool is_available(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *c = (struct ocpp_connector *)ctx;

	if (!is_csms_up(c)) { /* boot first */
		return false;
	}

	if (!is_state_x(&c->base, A)) {
		return false;
	}
	if (!is_session_active(c)) { /* second idTag or timeout */
		info("session %s", is_session_established(c)?
				"expired" : "not active");
		return true;
	}

	return false;
}

static bool is_preparing(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *c = (struct ocpp_connector *)ctx;

	if (!is_csms_up(c)) { /* boot first */
		return false;
	}

	return is_state_x(&c->base, B) ||
		(is_state_x(&c->base, A) && is_session_active(c));
}

static bool is_charging(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *c = (struct ocpp_connector *)ctx;

	if (state == Preparing) {
		return is_state_x(&c->base, B) && is_session_established(c) &&
			is_transaction_started(c);
	} else if (state == SuspendedEV) {
		return is_state_x2(&c->base, C) || is_state_x2(&c->base, D);
	}

	return is_state_x2(&c->base, C) || is_state_x2(&c->base, D) ||
		(is_session_established(c) && is_transaction_started(c));
}

static bool is_charging_rdy(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *c = (struct ocpp_connector *)ctx;

	if (state != Preparing) {
		return false;
	}

	return is_state_x(&c->base, B) && is_session_active(c) &&
		ocpp_count_pending_requests() == 0;
}

static bool is_suspendedEV(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *c = (struct ocpp_connector *)ctx;

	if (state != Charging && state != SuspendedEV) {
		return false;
	}
	return is_state_x(&c->base, B);
}

static bool is_finishing(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *c = (struct ocpp_connector *)ctx;
	struct ocpp_charger *charger = (struct ocpp_charger *)c->base.charger;

	if (state != SuspendedEV && state != Charging) {
		return false;
	}

	if (charger->reboot_required >= OCPP_CHARGER_REBOOT_REQUIRED_REMOTELY ||
			is_state_x(&c->base, A) || is_state_x(&c->base, F) ||
			!is_session_established(c)) { /* second idTag */
		return true;
	}

	return false;
}

static bool is_reserved(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	return false;
}

static bool is_unavailable(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *c = (struct ocpp_connector *)ctx;
	struct ocpp_charger *charger = (struct ocpp_charger *)c->base.charger;

	if (!is_csms_up(c)) { /* boot first */
		return false;
	}

	if (is_state_x(&c->base, F)) {
		return false;
	}

	return *c->unavailable || charger->param.unavailability.charger;
}

static bool is_faulted(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *c = (struct ocpp_connector *)ctx;
	if (is_evse_error(&c->base, (connector_state_t)s2cp(state))) {
		return true;
	}
	return false;
}

static void send_connector_status(struct ocpp_connector *c, void *opt)
{
	struct ocpp_charger *charger = (struct ocpp_charger *)c->base.charger;

	if (charger->status_report_required) {
		charger->status_report_required = false;
		csms_request(OCPP_MSG_STATUS_NOTIFICATION, c,
				(void *)CONNECTOR_0);
	}

	csms_request(OCPP_MSG_STATUS_NOTIFICATION, c, opt);
}

static void send_metering(struct ocpp_connector *c)
{
	ocpp_measurand_t flags;

	if ((flags = update_metering(c))) {
		csms_request(OCPP_MSG_METER_VALUES, c, (void *)flags);
	}

	if ((flags = update_metering_clock_aligned(c))) {
		csms_request(OCPP_MSG_METER_VALUES, c, (void *)flags);
	}
}

static void do_boot(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *c = (struct ocpp_connector *)ctx;
	stop_pwm(&c->base);
	csms_request(OCPP_MSG_BOOTNOTIFICATION, c, 0);
}

static void do_missing_transaction(fsm_state_t state, fsm_state_t next_state,
		void *ctx)
{
	if (state != Booting) {
		return;
	}

	struct ocpp_connector *c = (struct ocpp_connector *)ctx;
	struct ocpp_charger *charger = (struct ocpp_charger *)c->base.charger;

	if (charger->param.missing_transaction_id) {
		const ocpp_stop_reason_t reason = get_stop_reason(c, state);
		set_transaction_id(c, charger->param.missing_transaction_id);
		metering_get_energy(c->base.param.metering,
				&c->session.metering.stop_wh, NULL);
		csms_request(OCPP_MSG_STOP_TRANSACTION, c, (void *)reason);
		clear_session(c);
		charger->param.missing_transaction_id = 0;
	}
}

static void do_available(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *c = (struct ocpp_connector *)ctx;

	send_connector_status(c, NULL);
	clear_session(c);

	do_missing_transaction(state, next_state, ctx);
}

static void do_preparing(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *c = (struct ocpp_connector *)ctx;
	struct ocpp_charger *charger = (struct ocpp_charger *)c->base.charger;

	if (is_state_x(&c->base, A)) { /* remote start or idTag */
		uint32_t conn_timeout_sec;
		ocpp_get_configuration("ConnectionTimeOut",
				&conn_timeout_sec, sizeof(conn_timeout_sec), 0);
		set_session_current_expiry(c, c->now + conn_timeout_sec);
	}

	if (charger->status_report_required) {
		charger->status_report_required = false;
		csms_request(OCPP_MSG_STATUS_NOTIFICATION, c, (void *)CONNECTOR_0);
	}

        /* NOTE: Transition to the Preparing state to allow the user to restart
         * charging if desired, but maintain the Finishing state in the CSMS to
         * indicate that the user has not removed the connector after completing
         * the charge. */
	if (state != Finishing) {
		csms_request(OCPP_MSG_STATUS_NOTIFICATION, c, 0);
	}

	do_missing_transaction(state, next_state, ctx);
}

static void do_request_tid(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *c = (struct ocpp_connector *)ctx;
	metering_get_energy(c->base.param.metering,
			&c->session.metering.start_wh, NULL);
	csms_request(OCPP_MSG_START_TRANSACTION, c, 0);

	/* NOTE: This ensures the id is not maintained after unplugging. */
	set_session_current_expiry(c, c->now);
}

static void do_charging(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *c = (struct ocpp_connector *)ctx;

	if (state == Charging) {
		if (!is_supplying_power(&c->base)) {
			enable_power_supply(&c->base);
			send_connector_status(c, NULL);
		}
	} else {
		if (state == Preparing) {
			start_pwm(&c->base);
			return;
		}
	}

	send_metering(c);
}

static void do_suspendedEV(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *c = (struct ocpp_connector *)ctx;

	if (state == Charging) {
		disable_power_supply(&c->base);
	}

	send_connector_status(c, NULL);
}

static void do_finishing(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *c = (struct ocpp_connector *)ctx;

	if (state == Charging || state == SuspendedEV) {
		const ocpp_stop_reason_t reason = get_stop_reason(c, state);
		if (state == Charging) {
			disable_power_supply(&c->base);
		}
		metering_get_energy(c->base.param.metering,
				&c->session.metering.stop_wh, NULL);
		csms_request(OCPP_MSG_STOP_TRANSACTION, c, (void *)reason);
	}

	if (is_occupied(state)) {
		stop_pwm(&c->base);
	}

	send_connector_status(c, NULL);
	clear_session(c);
}

static void do_reserved(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *c = (struct ocpp_connector *)ctx;
	send_connector_status(c, NULL);
}

static void do_unavailable(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *c = (struct ocpp_connector *)ctx;

	send_connector_status(c, NULL);
	clear_session(c);

	do_missing_transaction(state, next_state, ctx);
}

static void do_faulted(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct ocpp_connector *c = (struct ocpp_connector *)ctx;
	union {
		void *opt;
		const char *str;
	} info = { .opt = NULL };

	if (is_emergency_stop(&c->base)) {
		info.str = "EmergencyStop";
	}

	send_connector_status(c, info.opt);
	clear_session(c);

	log_power_failure(&c->base);
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

const struct fsm_item *get_ocpp_fsm_table(size_t *transition_count)
{
	if (transition_count) {
		*transition_count = ARRAY_SIZE(transitions);
	}

	return transitions;
}
