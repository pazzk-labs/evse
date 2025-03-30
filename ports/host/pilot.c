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
#include "libmcu/compiler.h"

struct pilot *pilot_create(const struct pilot_params *params,
        struct adc122s051 *adc, struct lm_pwm_channel *pwm, uint16_t *buf)
{
	unused(params);
	unused(adc);
	unused(pwm);
	unused(buf);

	return 0;
}

void pilot_delete(struct pilot *pilot)
{
	unused(pilot);
}

void pilot_set_status_cb(struct pilot *pilot,
		pilot_status_cb_t cb, void *cb_ctx)
{
	unused(pilot);
	unused(cb);
	unused(cb_ctx);
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
	unused(pilot);
	unused(pct);
	return 0;
}

uint8_t pilot_get_duty_set(const struct pilot *pilot)
{
	unused(pilot);
	return 0;
}

uint8_t pilot_duty(const struct pilot *pilot)
{
	unused(pilot);
	return 0;
}

pilot_status_t pilot_status(const struct pilot *pilot)
{
	unused(pilot);
	return PILOT_STATUS_UNKNOWN;
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
	return false;
}

pilot_error_t pilot_error(const struct pilot *pilot)
{
	unused(pilot);
	return PILOT_ERROR_NONE;
}

const char *pilot_stringify_status(const pilot_status_t status)
{
	unused(status);
	return "Unknown";
}

void pilot_default_params(struct pilot_params *params)
{
	unused(params);
}
