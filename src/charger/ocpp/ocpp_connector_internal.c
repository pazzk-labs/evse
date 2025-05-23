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
#include "metering.h"
#include "logger.h"

static bool is_time_expired(const struct ocpp_connector *c)
{
	return c->session.timestamp.expiry &&
		c->session.timestamp.expiry <= c->now;
}

static bool update_metering_core(struct ocpp_connector *oc, time_t *timestamp,
		uint32_t interval, ocpp_reading_context_t context)
{
	struct connector *c = &oc->base;
	struct session_metering *v = &oc->session.metering;
	struct metering *meter = connector_meter(c);
	const time_t next = *timestamp + interval;

	if (oc->now == v->timestamp || interval == 0 || oc->now < next) {
		return false;
	}

	*timestamp = oc->now;
	v->timestamp = oc->now;
	v->context = context;

	metering_get_energy(meter, &v->wh, 0);
	metering_get_power(meter, &v->watt, 0);
	metering_get_current(meter, &v->milliamp);
	metering_get_voltage(meter, &v->millivolt);
	metering_get_power_factor(meter, &v->pf_centi);
	metering_get_frequency(meter, &v->centi_hertz);

	return true;
}

ocpp_connector_state_t ocpp_connector_state(struct ocpp_connector *oc)
{
	return (ocpp_connector_state_t)
		(int)fsm_get_state(&oc->base.fsm);
}

const char *ocpp_connector_stringify_state(ocpp_connector_state_t state)
{
	switch (state) {
	case Booting:       return "Booting";
	case Available:     return "Available";
	case Preparing:     return "Preparing";
	case Charging:      return "Charging";
	case SuspendedEV:   return "SuspendedEV";
	case SuspendedEVSE: return "SuspendedEVSE";
	case Finishing:     return "Finishing";
	case Reserved:      return "Reserved";
	case Unavailable:   return "Unavailable";
	case Faulted:       return "Faulted";
	default:            return "Unknown";
	}
}

connector_state_t
ocpp_connector_map_state_to_common(ocpp_connector_state_t state)
{
	const fsm_state_t tbl[] = {
		[Faulted]       = F,
		[Available]     = A,
		[Preparing]     = B,
		[Charging]      = C,
		[SuspendedEV]   = B,
		[SuspendedEVSE] = F,
		[Finishing]     = B,
		[Reserved]      = F,
		[Unavailable]   = F,
	};

	return (connector_state_t)tbl[state];
}

ocpp_status_t ocpp_connector_map_state_to_ocpp(ocpp_connector_state_t state)
{
	const ocpp_status_t tbl[] = {
		[Faulted]       = OCPP_STATUS_FAULTED,
		[Available]     = OCPP_STATUS_AVAILABLE,
		[Preparing]     = OCPP_STATUS_PREPARING,
		[Charging]      = OCPP_STATUS_CHARGING,
		[SuspendedEV]   = OCPP_STATUS_SUSPENDED_EV,
		[SuspendedEVSE] = OCPP_STATUS_SUSPENDED_EVSE,
		[Finishing]     = OCPP_STATUS_FINISHING,
		[Reserved]      = OCPP_STATUS_RESERVED,
		[Unavailable]   = OCPP_STATUS_UNAVAILABLE,
	};

	return tbl[state];
}

ocpp_error_t ocpp_connector_map_error_to_ocpp(connector_error_t error)
{
	switch (error) {
	case CONNECTOR_ERROR_NONE:    return OCPP_ERROR_NONE;
	case CONNECTOR_ERROR_EV_SIDE: return OCPP_ERROR_EV_COMMUNICATION;
	default:                      return OCPP_ERROR_OTHER;
	}
}

void ocpp_connector_set_availability(struct ocpp_connector *oc,
		const bool available)
{
	oc->checkpoint->unavailable = !available;
}

bool ocpp_connector_is_unavailable(const struct ocpp_connector *oc)
{
	return oc->checkpoint->unavailable;
}

bool ocpp_connector_is_csms_up(const struct ocpp_connector *oc)
{
	return oc->csms_up;
}

void ocpp_connector_set_csms_up(struct ocpp_connector *oc, const bool up)
{
	oc->csms_up = up;
}

bool ocpp_connector_is_session_established(const struct ocpp_connector *oc)
{
	const uint8_t empty[OCPP_ID_TOKEN_MAXLEN] = {0, };
	return memcmp(oc->session.auth.current.uid, empty, sizeof(empty)) != 0;
}

bool ocpp_connector_is_session_active(const struct ocpp_connector *oc)
{
	return ocpp_connector_is_session_established(oc) &&
		!is_time_expired(oc);
}

bool ocpp_connector_is_transaction_started(const struct ocpp_connector *oc)
{
	return oc->session.transaction_id != 0;
}

void ocpp_connector_clear_session(struct ocpp_connector *oc)
{
	memset(&oc->session, 0, sizeof(oc->session));
}

void ocpp_connector_set_session_current_expiry(struct ocpp_connector *oc,
		const time_t expiry)
{
	if (expiry) {
		oc->session.timestamp.expiry = expiry;
		info("expiry set to %lld, %llds from now",
				expiry, expiry - oc->now);
	}
}

void ocpp_connector_set_remote_reset_requested(struct ocpp_connector *oc,
		const bool requested, const bool forced)
{
	oc->remote_reset_requested = requested;
	oc->remote_reset_forced = forced;
}

bool ocpp_connector_is_remote_reset_requested(struct ocpp_connector *oc,
		bool *forced)
{
	if (forced) {
		*forced = oc->remote_reset_forced;
	}
	return oc->remote_reset_requested;
}

ocpp_stop_reason_t ocpp_connector_get_stop_reason(struct ocpp_connector *oc,
		ocpp_connector_state_t state)
{
	struct connector *c = &oc->base;

	if (state != Charging && state != SuspendedEV) {
		return OCPP_STOP_REASON_POWER_LOSS;
	} else if (connector_is_state_x(c, A)) {
		return OCPP_STOP_REASON_EV_DISCONNECTED;
	} else if (!ocpp_connector_is_session_established(oc)) {
		if (oc->session.remote_stop) {
			return OCPP_STOP_REASON_REMOTE;
		}
		return OCPP_STOP_REASON_LOCAL;
	} else if (connector_is_emergency_stop(c)) {
		return OCPP_STOP_REASON_EMERGENCY_STOP;
	} else if (oc->remote_reset_requested) {
		if (oc->remote_reset_forced) {
			return OCPP_STOP_REASON_HARD_RESET;
		} else {
			return OCPP_STOP_REASON_SOFT_RESET;
		}
	}
	return OCPP_STOP_REASON_OTHER;
}

void ocpp_connector_set_tid(struct ocpp_connector *oc,
		const ocpp_transaction_id_t transaction_id)
{
	oc->session.transaction_id = transaction_id;
}

void ocpp_connector_clear_checkpoint_tid(struct ocpp_connector *oc)
{
	oc->checkpoint->transaction_id = 0;
}

void ocpp_connector_set_checkpoint_tid(struct ocpp_connector *oc)
{
	oc->checkpoint->transaction_id = oc->session.transaction_id;
}

void ocpp_connector_raise_event(struct ocpp_connector *oc,
		const connector_event_t event)
{
	msgq_push(oc->evtq, &event, sizeof(event));
}

bool ocpp_connector_has_missing_transaction(struct ocpp_connector *oc)
{
	return oc->checkpoint->transaction_id != 0;
}

void ocpp_connector_set_session_uid(struct ocpp_connector *oc,
		const char uid[OCPP_ID_TOKEN_MAXLEN])
{
	memcpy(oc->session.auth.current.uid, uid,
			sizeof(oc->session.auth.current.uid));
}

void ocpp_connector_clear_session_id(struct auth *auth)
{
	memset(&auth->uid, 0, sizeof(auth->uid));
	memset(&auth->pid, 0, sizeof(auth->pid));
}

void ocpp_connector_set_session_pid(struct ocpp_connector *c,
		const char pid[OCPP_ID_TOKEN_MAXLEN])
{
	if (pid[0]) {
		memcpy(&c->session.auth.current.pid, pid,
				sizeof(c->session.auth.current.pid));
		info("parent \"%s\" authorized", pid);
	}
}

void ocpp_connector_set_session_pending(struct ocpp_connector *c,
		const char uid[OCPP_ID_TOKEN_MAXLEN])
{
	memcpy(&c->session.auth.trial.uid, uid, sizeof(c->session.auth.trial.uid));
	c->session.auth.timestamp = time(NULL);
	info("user \"%s\" in trial", uid);
}

void ocpp_connector_clear_session_pending(struct ocpp_connector *c)
{
	ocpp_connector_clear_session_id(&c->session.auth.trial);
}

bool ocpp_connector_is_session_pending(const struct ocpp_connector *oc)
{
	const uint8_t empty[OCPP_ID_TOKEN_MAXLEN] = {0, };
	return memcmp(oc->session.auth.trial.uid, empty, sizeof(empty)) != 0;
}

ocpp_measurand_t ocpp_connector_update_metering(struct ocpp_connector *oc)
{
	uint32_t sample_interval = 0;
	int sample_data_type = OCPP_MEASURAND_ENERGY_ACTIVE_IMPORT_REGISTER;

	ocpp_get_configuration("MeterValueSampleInterval",
			&sample_interval, sizeof(sample_interval), 0);
	ocpp_get_configuration("MeterValuesSampledData",
			&sample_data_type, sizeof(sample_data_type), 0);

	if (!update_metering_core(oc, &oc->session.metering.time_sample_periodic,
			sample_interval, OCPP_READ_CTX_SAMPLE_PERIODIC)) {
		return (ocpp_measurand_t)0;
	}

	return (ocpp_measurand_t)sample_data_type;
}

ocpp_measurand_t
ocpp_connector_update_metering_clock_aligned(struct ocpp_connector *oc)
{
	uint32_t clock_interval = 0;
	int clock_data_type = OCPP_MEASURAND_ENERGY_ACTIVE_IMPORT_REGISTER;

	ocpp_get_configuration("ClockAlignedDataInterval",
			&clock_interval, sizeof(clock_interval), 0);
	ocpp_get_configuration("MeterValuesAlignedData",
			&clock_data_type, sizeof(clock_data_type), 0);

	if (!clock_interval || (oc->now % clock_interval) != 0) {
		return (ocpp_measurand_t)0;
	}

	if (!update_metering_core(oc, &oc->session.metering.time_clock_periodic,
			clock_interval, OCPP_READ_CTX_SAMPLE_CLOCK)) {
		return (ocpp_measurand_t)0;
	}

	return (ocpp_measurand_t)clock_data_type;
}

bool ocpp_connector_has_transaction(struct connector *c,
		const ocpp_transaction_id_t tid)
{
	struct ocpp_connector *oc = (struct ocpp_connector *)c;
	return oc->session.transaction_id == tid ||
		oc->checkpoint->transaction_id == tid;
}

void ocpp_connector_set_auth_result_cb(struct ocpp_connector *oc,
		ocpp_session_result_cb_t cb, void *cb_ctx)
{
	oc->auth.cb = cb;
	oc->auth.cb_ctx = cb_ctx;
}

void ocpp_connector_clear_auth_result_cb(struct ocpp_connector *oc)
{
	oc->auth.cb = NULL;
	oc->auth.cb_ctx = NULL;
}

void ocpp_connector_dispatch_auth_result(struct ocpp_connector *oc,
		ocpp_session_result_t result)
{
	if (oc->auth.cb) {
		(*oc->auth.cb)(&oc->base, result, oc->auth.cb_ctx);
		oc->auth.cb = NULL;
	}
}

bool ocpp_connector_is_session_uid_equal(struct connector *c,
		const char uid[OCPP_ID_TOKEN_MAXLEN])
{
	struct ocpp_connector *oc = (struct ocpp_connector *)c;
	const uint8_t empty[OCPP_ID_TOKEN_MAXLEN] = {0, };
	return memcmp(uid, empty, sizeof(empty)) &&
		memcmp(oc->session.auth.current.uid, uid,
			sizeof(oc->session.auth.current.uid)) == 0;
}

bool ocpp_connector_is_session_pid_equal(struct connector *c,
		const char pid[OCPP_ID_TOKEN_MAXLEN])
{
	struct ocpp_connector *oc = (struct ocpp_connector *)c;
	const uint8_t empty[OCPP_ID_TOKEN_MAXLEN] = {0, };
	return memcmp(pid, empty, sizeof(empty)) != 0 &&
		memcmp(oc->session.auth.current.pid, pid,
			sizeof(oc->session.auth.current.pid)) == 0;
}
