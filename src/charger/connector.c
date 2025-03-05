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

#include "charger/connector.h"

#include <string.h>
#include <errno.h>

#include "iec61851.h"
#include "safety.h"
#include "metering.h"
#include "libmcu/metrics.h"
#include "libmcu/compiler.h"
#include "logger.h"

/* 1-second delay is required for initial stabilization of the power line safety
 * check as the safety module checks power line safety once per second.
 * Here we double this for enough delay time to ensure that a malfunction does
 * not cause charging interruption. */
#define INITIAL_STABILIZATION_SEC	(1*2)

#if !defined(MIN)
#define MIN(a, b)			(((a) > (b))? (b) : (a))
#endif

#if !defined(ARRAY_SIZE)
#define ARRAY_SIZE(x)			(sizeof(x) / sizeof((x)[0]))
#endif

static bool is_charging(connector_state_t state)
{
	return state == C || state == D;
}

static bool is_input_power_ok(struct connector *c)
{
	const uint8_t f = (uint8_t)c->param.input_frequency;
	const safety_status_t err = safety_status(SAFETY_TYPE_INPUT_POWER, f);

	if (err != SAFETY_STATUS_OK) {
		ratelim_request_format(&c->log_ratelim, logger_error,
				"input power: %d, %dHz", err,
				safety_get_frequency(SAFETY_TYPE_INPUT_POWER));
	}

	return err == SAFETY_STATUS_OK;
}

static bool is_output_power_ok(struct connector *c)
{
	const uint8_t f = (uint8_t)c->param.input_frequency;
	const safety_status_t err = safety_status(SAFETY_TYPE_OUTPUT_POWER, f);

	if (err != SAFETY_STATUS_OK) {
		ratelim_request_format(&c->log_ratelim, logger_error,
				"output power: %d, %dHz", err,
				safety_get_frequency(SAFETY_TYPE_OUTPUT_POWER));
	}

	return err == SAFETY_STATUS_OK;
}

static bool is_emergency_stop(const struct connector *c)
{
	const uint8_t freq = (uint8_t)c->param.input_frequency;
	return safety_status(SAFETY_TYPE_OUTPUT_POWER, freq)
		== SAFETY_STATUS_EMERGENCY_STOP;
}

static connector_state_t get_state_from_iec61851(const iec61851_state_t state)
{
	const connector_state_t tbl[] = {
		[IEC61851_STATE_A] = A,
		[IEC61851_STATE_B] = B,
		[IEC61851_STATE_C] = C,
		[IEC61851_STATE_D] = D,
		[IEC61851_STATE_E] = E,
		[IEC61851_STATE_F] = F,
	};
	return tbl[state];
}

static uint32_t calc_output_current(const struct connector *c)
{
	/* TODO: implement dynamic current calculation. */
	return c->param.max_output_current_mA;
}

static connector_state_t get_state(const struct connector *c)
{
	return get_state_from_iec61851(iec61851_state(c->param.iec61851));
}

static uint8_t get_target_duty(const struct connector *c)
{
	return iec61851_get_pwm_duty_target(c->param.iec61851);
}

connector_state_t connector_state(const struct connector *self)
{
	return get_state(self);
}

uint8_t connector_get_target_duty(const struct connector *self)
{
	return get_target_duty(self);
}

uint8_t connector_get_actual_duty(const struct connector *self)
{
	return iec61851_get_pwm_duty(self->param.iec61851);
}

void connector_start_duty(struct connector *self)
{
	const uint32_t current = calc_output_current(self);
	iec61851_set_current(self->param.iec61851, current);
}

void connector_stop_duty(struct connector *self)
{
	iec61851_set_current(self->param.iec61851, 0);
}

void connector_go_fault(struct connector *self)
{
	iec61851_set_state_f(self->param.iec61851);
}

void connector_enable_power_supply(struct connector *self)
{
	iec61851_start_power_supply(self->param.iec61851);
}

void connector_disable_power_supply(struct connector *self)
{
	iec61851_stop_power_supply(self->param.iec61851);
}

bool connector_is_supplying_power(const struct connector *self)
{
	return iec61851_is_supplying_power(self->param.iec61851);
}

bool connector_is_state_x(const struct connector *self,
		connector_state_t state)
{
	return get_state(self) == state;
}

bool connector_is_state_x2(const struct connector *self,
		connector_state_t state)
{
	const uint8_t duty = get_target_duty(self);
	return get_state(self) == state && duty > 0 && duty < 100;
}

bool connector_is_occupied_state(const connector_state_t state)
{
	const iec61851_state_t tbl[] = {
		[A] = IEC61851_STATE_A,
		[B] = IEC61851_STATE_B,
		[C] = IEC61851_STATE_C,
		[D] = IEC61851_STATE_D,
		[E] = IEC61851_STATE_E,
		[F] = IEC61851_STATE_F,
	};
	static_assert(ARRAY_SIZE(tbl) == Sn, "table size mismatch");
	return iec61851_is_occupied_state(tbl[state]);
}

bool connector_is_evse_error(struct connector *self, connector_state_t state)
{
	if (get_target_duty(self) == 0 ||
			!is_input_power_ok(self) ||
			is_emergency_stop(self)) {
		return true;
	}

	if (is_charging(state) && !is_output_power_ok(self)) {
		if ((time(NULL) - self->time_last_state_change) >
				INITIAL_STABILIZATION_SEC) {
			return true;
		}
	}

	return false;
}

bool connector_is_emergency_stop(const struct connector *self)
{
	return is_emergency_stop(self);
}

bool connector_is_input_power_ok(struct connector *self)
{
	return is_input_power_ok(self);
}

bool connector_is_output_power_ok(struct connector *self)
{
	return is_output_power_ok(self);
}

bool connector_is_ev_response_timeout(const struct connector *self,
		uint32_t elapsed_sec)
{
	return elapsed_sec >= IEC61851_EV_RESPONSE_TIMEOUT_SEC;
}

bool connector_is_enabled(const struct connector *self)
{
	return self->enabled;
}

bool connector_is_reserved(const struct connector *self)
{
	return self->reserved;
}

bool connector_set_reserved(struct connector *self)
{
	self->reserved = true;
	return true;
}

bool connector_clear_reserved(struct connector *self)
{
	self->reserved = false;
	return true;
}

bool connector_validate_param(const struct connector_param *param)
{
	if (!param || !param->name || !param->iec61851) {
		return false;
	}

	if (param->min_output_current_mA > param->max_output_current_mA) {
		return false;
	}

	return true;
}

const char *connector_stringify_state(const connector_state_t state)
{
	const char *tbl[] = {
		[A] = "A",
		[B] = "B",
		[C] = "C",
		[D] = "D",
		[E] = "E",
		[F] = "F",
	};
	return tbl[state];
}

connector_error_t connector_error(const struct connector *self)
{
	return self->error;
}

int connector_set_priority(struct connector *self, const int priority)
{
	self->param.priority = priority;
	return 0;
}

int connector_priority(const struct connector *connector)
{
	return connector->param.priority;
}

struct metering *connector_meter(const struct connector *self)
{
	return self->param.metering;
}

const char *connector_name(const struct connector *self)
{
	return self->param.name;
}

size_t connector_stringify_event(const connector_event_t event,
		char *buf, size_t bufsize)
{
	const char *tbl[] = {
		"Plugged",
		"Unplugged",
		"Charging Started",
		"Charging Suspended",
		"Charging Ended",
		"Error",
		"Recovery",
		"Billing Started",
		"Billing Updated",
		"Billing Ended",
		"Occupied",
		"Unoccupied",
		"Authorization Rejected",
		"Reserved",
		"Enabled",
	};

	size_t len = 0;

	for (connector_event_t e = CONNECTOR_EVENT_ENABLED; e; e >>= 1) {
		if (e & event) {
			const char *str = tbl[__builtin_ctz(e)];
			if (len) {
				strncpy(&buf[len], ",", bufsize - len - 1);
				len = MIN(len + 1, bufsize - 1);
			}
			strncpy(&buf[len], str, bufsize - len - 1);
			len = MIN(len + strlen(str), bufsize - 1);
		}
	}

	buf[len] = '\0';

	return len;
}

void connector_update_metrics(const connector_state_t state)
{
	const metric_key_t tbl[] = {
		[A] = ChargerStateACount,
		[B] = ChargerStateBCount,
		[C] = ChargerStateCCount,
		[D] = ChargerStateDCount,
		[E] = ChargerStateECount,
		[F] = ChargerStateFCount,
	};
	static_assert(ARRAY_SIZE(tbl) == Sn, "table size mismatch");
	metrics_increase(tbl[state]);
}

int connector_register_event_cb(struct connector *self,
		connector_event_cb_t cb, void *cb_ctx)
{
	self->event_cb = cb;
	self->event_cb_ctx = cb_ctx;

	return 0;
}

int connector_process(struct connector *self)
{
	if (!self->enabled) {
		return -ENOTCONN;
	}

	return self->api->process(self);
}

int connector_enable(struct connector *self)
{
	int err = self->api->enable(self);

	if (!err) {
		if (self->param.metering) {
			metering_enable(self->param.metering);
		}

		if (self->event_cb) {
			(*self->event_cb)(self, CONNECTOR_EVENT_ENABLED,
					self->event_cb_ctx);
		}

		self->time_last_state_change = time(NULL);
		self->enabled = true;
		info("connector \"%s\" enabled", self->param.name);
	}

	return err;
}

int connector_disable(struct connector *self)
{
	int err = self->api->disable(self);

	if (self->param.metering) {
		metering_disable(self->param.metering);
	}

	self->enabled = false;
	info("connector \"%s\" disabled", self->param.name);

	return err;
}
