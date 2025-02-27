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

#if !defined(DEFAULT_INPUT_CURRENT)
#define DEFAULT_INPUT_CURRENT			31818 /* milliampere */
#endif
#if !defined(DEFAULT_INPUT_VOLTAGE)
#define DEFAULT_INPUT_VOLTAGE			220 /* V */
#endif
#if !defined(DEFAULT_INPUT_FREQUENCY)
#define DEFAULT_INPUT_FREQUENCY			60 /* Hz */
#endif
#if !defined(DEFAULT_OUTPUT_CURRENT_MIN)
#define DEFAULT_OUTPUT_CURRENT_MIN		DEFAULT_INPUT_CURRENT
#endif
#if !defined(DEFAULT_OUTPUT_CURRENT_MAX)
#define DEFAULT_OUTPUT_CURRENT_MAX		DEFAULT_OUTPUT_CURRENT_MIN
#endif

#if !defined(MIN)
#define MIN(a, b)				(((a) > (b))? (b) : (a))
#endif

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

void charger_default_param(struct charger_param *param)
{
	if (param) {
		*param = (struct charger_param) {
			.max_input_current_mA = DEFAULT_INPUT_CURRENT,
			.input_voltage = DEFAULT_INPUT_VOLTAGE,
			.input_frequency = DEFAULT_INPUT_FREQUENCY,
			.max_output_current_mA = DEFAULT_OUTPUT_CURRENT_MAX,
			.min_output_current_mA = DEFAULT_OUTPUT_CURRENT_MIN,
		};
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

size_t charger_stringify_event(const charger_event_t event,
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
		"Reboot Required",
		"Configuration Changed",
		"Parameter Updated",
		"Billing Started",
		"Billing Updated",
		"Billing Ended",
		"Occupied",
		"Unoccupied",
		"Authorization Rejected",
		"Reserved",
	};

	size_t len = 0;

	for (charger_event_t e = CHARGER_EVENT_RESERVED; e; e >>= 1) {
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
