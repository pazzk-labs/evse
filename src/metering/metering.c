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

#include "metering.h"
#include <string.h>
#include "adapter/hlw8112.h"

typedef struct metering *(*metering_ctor)(const struct metering_param *param,
		metering_save_cb_t save_cb, void *save_cb_ctx);

static const metering_ctor ctors[] = {
	[METERING_HLW811X] = metering_create_hlw8112,
};

int metering_get_voltage(struct metering *self, int32_t *millivolt)
{
	return ((struct metering_api *)self)->get_voltage(self, millivolt);
}

int metering_get_current(struct metering *self, int32_t *milliamp)
{
	return ((struct metering_api *)self)->get_current(self, milliamp);
}

int metering_get_power_factor(struct metering *self, int32_t *centi)
{
	return ((struct metering_api *)self)->get_power_factor(self, centi);
}

int metering_get_frequency(struct metering *self, int32_t *centihertz)
{
	return ((struct metering_api *)self)->get_frequency(self, centihertz);
}

int metering_get_phase(struct metering *self, int32_t *centidegree, int hz)
{
	return ((struct metering_api *)self)->get_phase(self, centidegree, hz);
}

int metering_get_energy(struct metering *self, uint64_t *wh, uint64_t *varh)
{
	return ((struct metering_api *)self)->get_energy(self, wh, varh);
}

int metering_get_power(struct metering *self, int32_t *watt, int32_t *var)
{
	return ((struct metering_api *)self)->get_power(self, watt, var);
}

int metering_save_energy(struct metering *self)
{
	return ((struct metering_api *)self)->save_energy(self);
}

int metering_step(struct metering *self)
{
	return ((struct metering_api *)self)->step(self);
}

int metering_enable(struct metering *self)
{
	return ((struct metering_api *)self)->enable(self);
}

int metering_disable(struct metering *self)
{
	return ((struct metering_api *)self)->disable(self);
}

struct metering *metering_create(const metering_t type,
		const struct metering_param *param,
		metering_save_cb_t save_cb, void *save_cb_ctx)
{
	return ctors[type](param, save_cb, save_cb_ctx);
}

void metering_destroy(struct metering *self)
{
	((struct metering_api *)self)->destroy(self);
}
