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

#include "iec61851.h"
#include <stdlib.h>
#include "logger.h"

#define K			(1000)

struct iec61851 {
	struct pilot *pilot;
	struct relay *relay;

	uint8_t pilot_duty_pct;
	bool is_supplying_power;
};

static bool is_state_error(const iec61851_state_t state)
{
	return state == IEC61851_STATE_E || state == IEC61851_STATE_F;
}

static bool is_state_occupied(const iec61851_state_t state)
{
	return state == IEC61851_STATE_B ||
		state == IEC61851_STATE_C ||
		state == IEC61851_STATE_D;
}

static bool is_state_charging(const iec61851_state_t state)
{
	return state == IEC61851_STATE_C || state == IEC61851_STATE_D;
}

static void set_pilot(struct iec61851 *self, const uint8_t pct)
{
	pilot_set_duty(self->pilot, pct);
	self->pilot_duty_pct = pct;
	info("pilot duty: %d%%(%lumA)", pct, iec61851_duty_to_milliampere(pct));
}

iec61851_state_t iec61851_state(struct iec61851 *self)
{
	/* If measured duty does not match the set duty, the pilot is not
	 * functioning properly. */
	if (pilot_get_duty_set(self->pilot) == 0) {
		const uint8_t measured = pilot_duty(self->pilot);
		if (pilot_duty(self->pilot) != 0) {
			error("pilot duty mismatch: %u%% != 0%%", measured);
		}
		return IEC61851_STATE_F;
	}

	switch (pilot_status(self->pilot)) {
	case PILOT_STATUS_A: return IEC61851_STATE_A;
	case PILOT_STATUS_B: return IEC61851_STATE_B;
	case PILOT_STATUS_C: return IEC61851_STATE_C;
	case PILOT_STATUS_D: return IEC61851_STATE_D;
	case PILOT_STATUS_E: return IEC61851_STATE_E;
	case PILOT_STATUS_F: return IEC61851_STATE_F;
	default:             return IEC61851_STATE_F;
	}
}

void iec61851_stop_pwm(struct iec61851 *self)
{
	set_pilot(self, 100);
}

void iec61851_start_pwm(struct iec61851 *self, const uint8_t pct)
{
	set_pilot(self, pct);
}

void iec61851_set_current(struct iec61851 *self, const uint32_t milliampere)
{
	set_pilot(self, iec61851_milliampere_to_duty(milliampere));
}

void iec61851_set_state_f(struct iec61851 *self)
{
	set_pilot(self, 0);
}

uint8_t iec61851_get_pwm_duty(struct iec61851 *self)
{
	return pilot_duty(self->pilot);
}

uint8_t iec61851_get_pwm_duty_set(struct iec61851 *self)
{
	return self->pilot_duty_pct;
}

void iec61851_start_power_supply(struct iec61851 *self)
{
	relay_turn_on(self->relay);
	self->is_supplying_power = true;
	info("power supply to the EV started");
}

void iec61851_stop_power_supply(struct iec61851 *self)
{
	relay_turn_off(self->relay);
	self->is_supplying_power = false;
	info("power supply to the EV stopped");
}

bool iec61851_is_supplying_power(struct iec61851 *self)
{
	return self->is_supplying_power;
}

bool iec61851_is_charging(struct iec61851 *self)
{
	return is_state_charging(iec61851_state(self));
}

bool iec61851_is_charging_state(const iec61851_state_t state)
{
	return is_state_charging(state);
}

bool iec61851_is_occupied(struct iec61851 *self)
{
	return is_state_occupied(iec61851_state(self));
}

bool iec61851_is_occupied_state(const iec61851_state_t state)
{
	return is_state_occupied(state);
}

bool iec61851_is_error(struct iec61851 *self)
{
	return is_state_error(iec61851_state(self));
}

bool iec61851_is_error_state(const iec61851_state_t state)
{
	return is_state_error(state);
}

const char *iec61851_stringify_state(const iec61851_state_t state)
{
	switch (state) {
	case IEC61851_STATE_A:  return "A";
	case IEC61851_STATE_B:  return "B";
	case IEC61851_STATE_C:  return "C";
	case IEC61851_STATE_D:  return "D";
	case IEC61851_STATE_E:  return "E";
	case IEC61851_STATE_F:  return "F";
	default:                return "UNKNOWN";
	}
}

uint32_t iec61851_duty_to_milliampere(const uint8_t duty)
{
	uint32_t milliampere = 0;

	/* IEC61851-1 Table A.7 and A.8 */
	if (duty > 97) { /* 97% < duty <= 100% */
	} else if (duty > 96) { /* 96% < duty <= 97% */
		milliampere = 80 * K;
	} else if (duty > 85) { /* 85% < duty <= 96% */
		milliampere = ((uint32_t)duty - 64) * 25 * K / 10;
	} else if (duty >= 10) { /* 10% <= duty <= 85% */
		milliampere = (uint32_t)duty * 6 * K / 10;
	} else if (duty >= 8) { /* 8% <= duty < 10% */
		milliampere = 6 * K;
	}

	return milliampere;
}

uint8_t iec61851_milliampere_to_duty(const uint32_t milliampere)
{
	uint8_t duty = 100;

	if (milliampere > 80 * K) {
		duty = 96;
	} else if (milliampere >= 55 * K) { /* 51A < A <= 80A */
		duty = (uint8_t)(milliampere * 10 / K / 25 + 64);
	} else if (milliampere >= 6 * K) { /* 6A <= A <= 51A */
		duty = (uint8_t)(milliampere * 10 / 6 / K);
	}

	return duty;
}

struct iec61851 *iec61851_create(struct pilot *pilot, struct relay *relay)
{
	struct iec61851 *self;

	if (!(self = (struct iec61851 *)malloc(sizeof(*self)))) {
		return NULL;
	}

	*self = (struct iec61851) {
		.pilot = pilot,
		.relay = relay,
	};

	return self;
}

void iec61851_destroy(struct iec61851 *self)
{
	if (!self) {
		return;
	}

	free(self);
}
