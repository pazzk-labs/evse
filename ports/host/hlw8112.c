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

#include "../../src/metering/adapter/hlw8112.h"
#include <errno.h>
#include "libmcu/compiler.h"

struct metering {
	struct metering_api api;
};

static int get_voltage(struct metering *self, int32_t *millivolt)
{
	unused(self);
	unused(millivolt);
	return -ENOTSUP;
}

static int get_current(struct metering *self, int32_t *milliamp)
{
	unused(self);
	unused(milliamp);
	return -ENOTSUP;
}

static int get_power_factor(struct metering *self, int32_t *centi)
{
	unused(self);
	unused(centi);
	return -ENOTSUP;
}

static int get_frequency(struct metering *self, int32_t *centihertz)
{
	unused(self);
	unused(centihertz);
	return -ENOTSUP;
}

static int get_phase(struct metering *self, int32_t *centidegree, int hz)
{
	unused(self);
	unused(centidegree);
	unused(hz);
	return -ENOTSUP;
}

static int get_energy(struct metering *self, uint64_t *wh, uint64_t *varh)
{
	unused(self);
	unused(wh);
	unused(varh);
	return -ENOTSUP;
}

static int save_energy(struct metering *self)
{
	unused(self);
	return -ENOTSUP;
}

static int get_power(struct metering *self, int32_t *watt, int32_t *var)
{
	unused(self);
	unused(watt);
	unused(var);
	return -ENOTSUP;
}

static int step(struct metering *self)
{
	unused(self);
	return -ENOTSUP;
}

static int enable(struct metering *self)
{
	unused(self);
	return -ENOTSUP;
}

static int disable(struct metering *self)
{
	unused(self);
	return -ENOTSUP;
}

static void destroy(struct metering *self)
{
	unused(self);
}

static const struct metering_api *get_api(void)
{
	static const struct metering_api api = {
		.destroy = destroy,
		.enable = enable,
		.disable = disable,
		.step = step,
		.save_energy = save_energy,
		.get_voltage = get_voltage,
		.get_current = get_current,
		.get_power_factor = get_power_factor,
		.get_frequency = get_frequency,
		.get_phase = get_phase,
		.get_energy = get_energy,
		.get_power = get_power,
	};

	return &api;
}

struct metering *metering_create_hlw8112(const struct metering_param *param,
		metering_save_cb_t save_cb, void *save_cb_ctx)
{
	unused(param);
	unused(save_cb);
	unused(save_cb_ctx);

	static struct metering metering;

	metering.api = *get_api();

	return &metering;
}
