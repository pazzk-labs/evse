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
#include "charger_private.h"
#include "iec61851.h"
#include "safety.h"
#include "logger.h"

/* 1-second delay is required for initial stabilization of the power line safety
 * check as the safety module checks power line safety once per second.
 * Here we double this for enough delay time to ensure that a malfunction does
 * not cause charging interruption. */
#define INITIAL_STABILIZATION_SEC	(1*2)

static bool is_charging(connector_state_t state)
{
	return state == C || state == D;
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

static iec61851_state_t get_iec61851_from_state(const connector_state_t state)
{
	const iec61851_state_t tbl[] = {
		[A] = IEC61851_STATE_A,
		[B] = IEC61851_STATE_B,
		[C] = IEC61851_STATE_C,
		[D] = IEC61851_STATE_D,
		[E] = IEC61851_STATE_E,
		[F] = IEC61851_STATE_F,
	};
	return tbl[state];
}

static uint32_t calc_output_current(const struct connector *c)
{
	/* TODO: implement dynamic current calculation. */
	return c->param.max_output_current_mA;
}

connector_state_t get_state(const struct connector *c)
{
	return get_state_from_iec61851(iec61851_state(c->param.iec61851));
}

uint8_t get_pwm_duty_set(const struct connector *c)
{
	return iec61851_get_pwm_duty_set(c->param.iec61851);
}

uint8_t read_pwm_duty(const struct connector *c)
{
	return iec61851_get_pwm_duty(c->param.iec61851);
}

void start_pwm(struct connector *c)
{
	const uint32_t current = calc_output_current(c);
	iec61851_set_current(c->param.iec61851, current);
}

void stop_pwm(struct connector *c)
{
	iec61851_set_current(c->param.iec61851, 0);
}

void go_fault(struct connector *c)
{
	iec61851_set_state_f(c->param.iec61851);
}

void enable_power_supply(struct connector *c)
{
	iec61851_start_power_supply(c->param.iec61851);
}

void disable_power_supply(struct connector *c)
{
	iec61851_stop_power_supply(c->param.iec61851);
}

bool is_state_x(const struct connector *c, connector_state_t state)
{
	return get_state(c) == state;
}

bool is_state_x2(const struct connector *c, connector_state_t state)
{
	const uint8_t duty = get_pwm_duty_set(c);
	return is_state_x(c, state) && duty > 0 && duty < 100;
}

bool is_evse_error(const struct connector *c, connector_state_t state)
{
	if (get_pwm_duty_set(c) == 0 || !is_input_power_ok(c) ||
			is_emergency_stop(c)) {
		return true;
	}

	if (is_charging(state) && !is_output_power_ok(c)) {
		const time_t now = time(NULL);
		if ((now - c->time_last_state_change) >
				INITIAL_STABILIZATION_SEC) {
			return true;
		}
	}

	return false;
}

bool is_supplying_power(const struct connector *c)
{
	return iec61851_is_supplying_power(c->param.iec61851);
}

bool is_emergency_stop(const struct connector *c)
{
	const uint8_t freq = (uint8_t)c->charger->param.input_frequency;
	return safety_status(SAFETY_TYPE_OUTPUT_POWER, freq)
		== SAFETY_STATUS_EMERGENCY_STOP;
}

bool is_input_power_ok(const struct connector *c)
{
	const uint8_t f = (uint8_t)c->charger->param.input_frequency;
	const safety_status_t err = safety_status(SAFETY_TYPE_INPUT_POWER, f);

	return err == SAFETY_STATUS_OK;
}

bool is_output_power_ok(const struct connector *c)
{
	const uint8_t f = (uint8_t)c->charger->param.input_frequency;
	const safety_status_t err = safety_status(SAFETY_TYPE_OUTPUT_POWER, f);

	return err == SAFETY_STATUS_OK;
}

bool is_early_recovery(uint32_t elapsed_sec)
{
	return elapsed_sec < IEC61851_EV_RESPONSE_TIMEOUT_SEC;
}

charger_event_t get_event_from_state_change(const connector_state_t new_state,
		const connector_state_t old_state)
{
	const charger_event_t tbl[] = {
		[A] = CHARGER_EVENT_NONE,
		[B] = CHARGER_EVENT_PLUGGED,
		[C] = CHARGER_EVENT_CHARGING_STARTED,
		[D] = CHARGER_EVENT_CHARGING_STARTED,
		[E] = CHARGER_EVENT_ERROR,
		[F] = CHARGER_EVENT_ERROR,
	};
	charger_event_t events = tbl[new_state];
	const iec61851_state_t prev = get_iec61851_from_state(old_state);

	if (iec61851_is_occupied_state(prev)) {
		if (new_state == A) {
			events |= CHARGER_EVENT_UNPLUGGED;
		}
		if (iec61851_is_charging_state(prev)) {
			if (new_state == B) {
				events = CHARGER_EVENT_CHARGING_SUSPENDED;
			} else {
				events |= CHARGER_EVENT_CHARGING_ENDED;
			}
		}
	} else if (old_state == F) {
		events |= CHARGER_EVENT_ERROR_RECOVERY;
	}

	return events;
}

const char *stringify_state(const connector_state_t state)
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

void log_power_failure(const struct connector *c)
{
	const uint8_t freq = (uint8_t)c->charger->param.input_frequency;
	safety_status_t err;

	if ((err = safety_status(SAFETY_TYPE_INPUT_POWER, freq))
			!= SAFETY_STATUS_OK) {
		ratelim_request_format(&c->charger->log_ratelim, logger_error,
				"input power: %d, %dHz", err,
				safety_get_frequency(SAFETY_TYPE_INPUT_POWER));
	}

	if ((err = safety_status(SAFETY_TYPE_OUTPUT_POWER, freq))
			!= SAFETY_STATUS_OK) {
		ratelim_request_format(&c->charger->log_ratelim, logger_error,
				"output power: %d, %dHz", err,
				safety_get_frequency(SAFETY_TYPE_OUTPUT_POWER));
	}
}
