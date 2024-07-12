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

#include "charger_private.h"
#include "charger/connector.h"
#include <string.h>
#include <errno.h>
#include "logger.h"

static bool validate_charger_param(const struct charger_param *param)
{
	if (!param) {
		return false;
	}

	if (param->max_input_current_mA == 0 ||
			param->input_voltage == 0 ||
			param->input_frequency == 0) {
		return false;
	}

	return true;
}

static void destroy_connectors(struct charger *charger)
{
	struct list *p;
	struct list *n;

	list_for_each_safe(p, n, &charger->connectors.list) {
		charger->api.destroy_connector(charger, p);
		charger->connectors.count--;
	}
}

int charger_register_event_cb(struct charger *charger,
		charger_event_cb_t cb, void *cb_ctx)
{
	charger->event_cb = cb;
	charger->event_cb_ctx = cb_ctx;

	return 0;
}

struct connector *charger_get_connector(struct charger *charger,
		const char *connector_name)
{
	return get_connector(charger, connector_name);
}

struct connector *charger_get_connector_free(struct charger *charger)
{
	return get_connector_free(charger);
}

struct connector *charger_get_connector_by_id(struct charger *charger,
		const int id)
{
	return get_connector_by_id(charger, id);
}

bool charger_create_connector(struct charger *charger,
		const struct connector_param *param)
{
	struct connector *c;

	if (!connector_validate_param(param,
			charger->param.max_input_current_mA)) {
		error("invalid connector param");
		return false;
	}

	if (!(c = charger->api.create_connector(charger, param))) {
		return false;
	}

	charger->connectors.count++;

	return true;
}

int charger_step(struct charger *self, uint32_t *next_period_ms)
{
	return self->api.step(self, next_period_ms);
}

int charger_init(struct charger *charger, const struct charger_param *param)
{
	if (!validate_charger_param(param)) {
		return -EINVAL;
	}

	memcpy(&charger->param, param, sizeof(charger->param));
	list_init(&charger->connectors.list);

	ratelim_init(&charger->log_ratelim, RATELIM_UNIT_SECOND,
			CHARGER_LOG_RATE_CAP, CHARGER_LOG_RATE_SEC);

	return 0;
}

int charger_deinit(struct charger *charger)
{
	destroy_connectors(charger);

	return 0;
}
