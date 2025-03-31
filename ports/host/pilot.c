/*
 * This file is part of the Pazzk project <https://pazzk.net/>.
 * Copyright (c) 2025 Pazzk <team@pazzk.net>.
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

#include "pilot.h"
#include <stdlib.h>
#include <string.h>
#include "libmcu/compiler.h"

struct pilot {
	uint8_t duty_pct;
	pilot_status_t status;

	pilot_status_cb_t cb;
	void *cb_ctx;
};

struct pilot *pilot_create(const struct pilot_params *params,
        struct adc122s051 *adc, struct lm_pwm_channel *pwm, uint16_t *buf)
{
	struct pilot *pilot = (struct pilot *)malloc(sizeof(*pilot));
	memset(pilot, 0, sizeof(*pilot));

	unused(params);
	unused(adc);
	unused(pwm);
	unused(buf);

	return pilot;
}

void pilot_delete(struct pilot *pilot)
{
	free(pilot);
}

void pilot_set_status_cb(struct pilot *pilot,
		pilot_status_cb_t cb, void *cb_ctx)
{
	pilot->cb = cb;
	pilot->cb_ctx = cb_ctx;
}

int pilot_enable(struct pilot *pilot)
{
	unused(pilot);
	return 0;
}

int pilot_disable(struct pilot *pilot)
{
	unused(pilot);
	return 0;
}

int pilot_set_duty(struct pilot *pilot, const uint8_t pct)
{
	pilot->duty_pct = pct;
	return 0;
}

uint8_t pilot_get_duty_set(const struct pilot *pilot)
{
	return pilot->duty_pct;
}

uint8_t pilot_duty(const struct pilot *pilot)
{
	return pilot->duty_pct;
}

#if defined(HOST_BUILD)
void pilot_set_status(struct pilot *pilot, const pilot_status_t status)
{
	pilot->status = status;
}
#endif

pilot_status_t pilot_status(const struct pilot *pilot)
{
	return pilot->status;
}

uint16_t pilot_millivolt(const struct pilot *pilot, const bool low_voltage)
{
	unused(pilot);
	unused(low_voltage);
	return 0;
}

bool pilot_ok(const struct pilot *pilot)
{
	unused(pilot);
	return true;
}

pilot_error_t pilot_error(const struct pilot *pilot)
{
	unused(pilot);
	return PILOT_ERROR_NONE;
}

const char *pilot_stringify_status(const pilot_status_t status)
{
	switch (status) {
	case PILOT_STATUS_A:
		return "A(12V)";
	case PILOT_STATUS_B:
		return "B(9V)";
	case PILOT_STATUS_C:
		return "C(6V)";
	case PILOT_STATUS_D:
		return "D(3V)";
	case PILOT_STATUS_E:
		return "E(0V)";
	case PILOT_STATUS_F:
		return "F(-12V)";
	default:
		return "Unknown";
	}
}

void pilot_default_params(struct pilot_params *params)
{
	*params = (struct pilot_params) {
		.scan_interval_ms = 10,
		.cutoff_voltage_mv = 1996,
		.noise_tolerance_mv = 50,
		.max_transition_clocks = 15,
		.boundary = {
			.upward = {
				.a = 3038,
				.b = 2718,
				.c = 2397,
				.d = 2076,
				.e = 767,
			},
			.downward = {
				.a = 2985,
				.b = 2644,
				.c = 2344,
				.d = 2022,
				.e = 767,
			},
		},
		.sample_count = 500,
	};
}
