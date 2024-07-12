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
#include "charger/ocpp.h"
#include "logger.h"

static bool update_metering_core(struct ocpp_connector *c, time_t *timestamp,
		uint32_t interval, ocpp_reading_context_t context)
{
	struct session_metering *v = &c->session.metering;
	const time_t next = *timestamp + interval;

	if (c->now == v->timestamp || interval == 0 || c->now < next) {
		return false;
	}

	*timestamp = c->now;
	v->timestamp = c->now;
	v->context = context;

	metering_get_energy(c->base.param.metering, &v->wh, 0);
	metering_get_power(c->base.param.metering, &v->watt, 0);
	metering_get_current(c->base.param.metering, &v->milliamp);
	metering_get_voltage(c->base.param.metering, &v->millivolt);
	metering_get_power_factor(c->base.param.metering, &v->pf_centi);
	metering_get_frequency(c->base.param.metering, &v->centi_hertz);

	return true;
}

static bool is_time_expired(const struct ocpp_connector *c)
{
	return c->session.timestamp.expiry <= c->now;
}

bool is_occupied(const ocpp_connector_status_t status)
{
	return status == Preparing || status == Charging ||
		status == SuspendedEV || status == Finishing;
}

bool is_csms_up(const struct ocpp_connector *c)
{
	struct ocpp_charger *charger = (struct ocpp_charger *)c->base.charger;
	return charger->csms_up;
}

bool is_session_established(const struct ocpp_connector *c)
{
	const uint8_t empty[OCPP_ID_TOKEN_MAXLEN] = {0, };
	return memcmp(c->session.auth.current.id, empty, sizeof(empty)) != 0;
}

bool is_session_active(const struct ocpp_connector *c)
{
	return is_session_established(c) && !is_time_expired(c);
}

bool is_session_trial_id_exist(const struct ocpp_connector *c)
{
	const uint8_t empty[OCPP_ID_TOKEN_MAXLEN] = {0, };
	return memcmp(c->session.auth.trial.id, empty, sizeof(empty)) != 0;
}

bool is_transaction_started(const struct ocpp_connector *c)
{
	return c->session.transaction_id != 0;
}

void clear_session_id(struct auth *auth)
{
	memset(&auth->id, 0, sizeof(auth->id));
	memset(&auth->parent_id, 0, sizeof(auth->parent_id));
}

void clear_session(struct ocpp_connector *c)
{
	memset(&c->session, 0, sizeof(c->session));
}

void set_transaction_id(struct ocpp_connector *c,
		const ocpp_charger_transaction_id_t transaction_id)
{
	c->session.transaction_id = transaction_id;
}

void set_session_current_expiry(struct ocpp_connector *c, const time_t expiry)
{
	if (expiry) {
		c->session.timestamp.expiry = expiry;
	}
}

void set_session_parent_id(struct ocpp_connector *c,
		const char id[OCPP_ID_TOKEN_MAXLEN])
{
	if (id[0]) {
		memcpy(&c->session.auth.current.parent_id, id,
				sizeof(c->session.auth.current.parent_id));
		info("parent \"%s\" authorized", id);
	}
}

void set_session_current_id(struct ocpp_connector *c,
		const char id[OCPP_ID_TOKEN_MAXLEN])
{
	int timeout = 0;
	ocpp_get_configuration("ConnectionTimeOut",
			&timeout, sizeof(timeout), NULL);
	memcpy(&c->session.auth.current.id, id,
			sizeof(c->session.auth.current.id));
	set_session_current_expiry(c, c->now + timeout);

	clear_session_id(&c->session.auth.trial);
	info("user \"%s\" authorized", c->session.auth.current.id);
}

void set_session_trial_id(struct ocpp_connector *c,
		const char id[OCPP_ID_TOKEN_MAXLEN])
{
	memcpy(&c->session.auth.trial.id, id, sizeof(c->session.auth.trial.id));

	info("user \"%s\" in trial", id);
}

void raise_event(struct ocpp_connector *c, const charger_event_t event)
{
	struct charger *charger = c->base.charger;
	if (charger->event_cb) {
		charger->event_cb(charger, &c->base, event,
				charger->event_cb_ctx);
	}
}

void accept_session_trial_id(struct ocpp_connector *c)
{
	set_session_current_id(c, (const char *)c->session.auth.trial.id);
}

ocpp_measurand_t update_metering(struct ocpp_connector *c)
{
	uint32_t sample_interval = 0;
	int sample_data_type = OCPP_MEASURAND_ENERGY_ACTIVE_IMPORT_REGISTER;

	ocpp_get_configuration("MeterValueSampleInterval",
			&sample_interval, sizeof(sample_interval), 0);
	ocpp_get_configuration("MeterValuesSampledData",
			&sample_data_type, sizeof(sample_data_type), 0);

	if (!update_metering_core(c, &c->session.metering.time_sample_periodic,
			sample_interval, OCPP_READ_CTX_SAMPLE_PERIODIC)) {
		return 0;
	}

	return (ocpp_measurand_t)sample_data_type;
}

ocpp_measurand_t update_metering_clock_aligned(struct ocpp_connector *c)
{
	uint32_t clock_interval = 0;
	int clock_data_type = OCPP_MEASURAND_ENERGY_ACTIVE_IMPORT_REGISTER;

	ocpp_get_configuration("ClockAlignedDataInterval",
			&clock_interval, sizeof(clock_interval), 0);
	ocpp_get_configuration("MeterValuesAlignedData",
			&clock_data_type, sizeof(clock_data_type), 0);

	if (!clock_interval || (c->now % clock_interval) != 0) {
		return 0;
	}

	if (!update_metering_core(c, &c->session.metering.time_clock_periodic,
			clock_interval, OCPP_READ_CTX_SAMPLE_CLOCK)) {
		return 0;
	}

	return (ocpp_measurand_t)clock_data_type;
}

ocpp_connector_status_t get_status(const struct ocpp_connector *c)
{
	return fsm_get_state(&c->fsm);
}

const char *stringify_status(ocpp_connector_status_t status)
{
	switch (status) {
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

fsm_state_t s2cp(ocpp_connector_status_t status)
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

	return tbl[status];
}

ocpp_status_t s2ocpp(ocpp_connector_status_t status)
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

	return tbl[status];
}

ocpp_error_t e2ocpp(connector_error_t error)
{
	switch (error) {
	case CONNECTOR_ERROR_NONE:    return OCPP_ERROR_NONE;
	case CONNECTOR_ERROR_EV_SIDE: return OCPP_ERROR_EV_COMMUNICATION;
	default:                      return OCPP_ERROR_OTHER;
	}
}

ocpp_stop_reason_t get_stop_reason(struct ocpp_connector *c,
		ocpp_connector_status_t status)
{
	struct ocpp_charger *charger = (struct ocpp_charger *)c->base.charger;

	if (status != Charging && status != SuspendedEV) {
		return OCPP_STOP_REASON_POWER_LOSS;
	} else if (is_state_x(&c->base, A)) {
		return OCPP_STOP_REASON_EV_DISCONNECTED;
	} else if (!is_session_established(c)) {
		if (c->session.remote_stop) {
			return OCPP_STOP_REASON_REMOTE;
		}
		return OCPP_STOP_REASON_LOCAL;
	} else if (is_emergency_stop(&c->base)) {
		return OCPP_STOP_REASON_EMERGENCY_STOP;
	} else if (charger->reboot_required) {
		return (!charger->remote_request)? OCPP_STOP_REASON_REBOOT :
			(charger->reboot_required == OCPP_CHARGER_REBOOT_FORCED)?
			OCPP_STOP_REASON_HARD_RESET : OCPP_STOP_REASON_SOFT_RESET;
	}
	return OCPP_STOP_REASON_OTHER;
}

struct ocpp_connector *get_connector_by_message_id(struct ocpp_charger *self,
		const uint8_t *msgid, const size_t msgid_len)
{
	struct list *p;

	list_for_each(p, &self->base.connectors.list) {
		struct ocpp_connector *c =
			list_entry(p, struct ocpp_connector, base.link);
		if (memcmp(c->ocpp_info.msgid, msgid, msgid_len) == 0) {
			return c;
		}
	}

	return NULL;
}

struct ocpp_connector *get_connector_by_transaction_id(struct ocpp_charger *self,
		const ocpp_charger_transaction_id_t transaction_id)
{
	struct list *p;

	list_for_each(p, &self->base.connectors.list) {
		struct ocpp_connector *c =
			list_entry(p, struct ocpp_connector, base.link);
		if (c->session.transaction_id == transaction_id) {
			return c;
		}
	}

	return NULL;
}
