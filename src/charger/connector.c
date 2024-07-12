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

connector_error_t connector_error(const struct connector *connector)
{
	return connector->error;
}

int connector_set_priority(struct connector *connector, const int priority)
{
	connector->param.priority = priority;
	return 0;
}

const char *connector_status(const struct connector *connector)
{
	return stringify_state(get_state(connector));
}

bool connector_validate_param(const struct connector_param *param,
		const uint32_t max_input_current)
{
	if (!param || !param->name || !param->iec61851) {
		return false;
	}

	if (param->min_output_current_mA > param->max_output_current_mA ||
		param->max_output_current_mA > max_input_current) {
		return false;
	}

	return true;
}
