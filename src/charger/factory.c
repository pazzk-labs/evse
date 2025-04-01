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

#include "charger/factory.h"

#include <string.h>

#include "libmcu/compiler.h"

#include "charger/free.h"
#include "charger/ocpp.h"
#include "charger/free_connector.h"
#include "charger/ocpp_connector.h"
#include "config.h"
#include "logger.h"

static_assert(sizeof(struct charger_param)
		== sizeof(((struct config *)0)->charger.param),
		"charger_param size mismatch");

typedef struct charger *(*charger_factory_t)(void *ctx);
typedef struct connector *
	(*connector_factory_t)(const struct connector_param *param);
typedef struct charger_extension *(*charger_extension_factory_t)(void);

static const char *get_charger_mode(void)
{
	static char mode[CONFIG_CHARGER_MODE_MAXLEN] = { 0, };
	if (config_get("chg.mode", mode, sizeof(mode)) < 0) {
		strncpy(mode, "free", sizeof(mode));
	}
	return mode;
}

const char *charger_factory_mode(void)
{
	return get_charger_mode();
}

struct charger *charger_factory_create(struct charger_param *param,
		struct charger_extension **extension, void *ctx)
{
	charger_factory_t charger_factory;
	const char *mode = get_charger_mode();

	config_get("chg.param", param, sizeof(*param));

	if (!charger_is_param_valid(param)) {
		charger_default_param(param);
		config_set("chg.param", param, sizeof(*param));
	}

	if (strcmp(mode, "ocpp") == 0) {
		charger_factory = ocpp_charger_create;
		*extension = ocpp_charger_extension();
	} else {
		charger_factory = free_charger_create;
		*extension = NULL;
	}

	info("%s charger created", mode);

	return (*charger_factory)(ctx);
}

struct connector *connector_factory_create(const struct connector_param *param)
{
	connector_factory_t connector_factory;
	const char *mode = get_charger_mode();

	if (strcmp(mode, "ocpp") == 0) {
		connector_factory = ocpp_connector_create;
	} else {
		connector_factory = free_connector_create;
	}

	return (*connector_factory)(param);
}
