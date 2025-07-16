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

#include "helper.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "libmcu/compiler.h"
#include "charger/charger.h"
#include "../charger/ocpp/ocpp_connector_internal.h"
#include "app.h"

struct ctx {
	const struct cli_io *io;
	struct app *app;
};

static void print_ocpp_configurations(const struct cli_io *io)
{
#define CFGVAL_STR_MAXLEN		256
	char buf[CFGVAL_STR_MAXLEN];

	for (size_t i = 0; i < ocpp_count_configurations(); i++) {
		const char *key;

		if (!(key = ocpp_get_configuration_keystr_from_index((int)i)) ||
				!ocpp_is_configuration_readable(key)) {
			continue;
		}

		const size_t value_size = ocpp_get_configuration_size(key);
		uint8_t value[value_size];
		bool readonly;

		ocpp_get_configuration(key, value, value_size, &readonly);
		ocpp_stringify_configuration_value(key, buf, sizeof(buf));

		printini(io, key, buf);
	}
}

static void on_each_connector(struct connector *c, void *ctx)
{
	struct ctx *p = (struct ctx *)ctx;
	struct ocpp_connector *oc = (struct ocpp_connector *)c;
	printini(p->io, "state", ocpp_connector_stringify_state(ocpp_connector_state(oc)));
}

static void do_info(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	unused(argc);
	unused(argv);
	unused(cmd);

	struct ctx *p = (struct ctx *)ctx;

	charger_iterate_connectors(p->app->charger, on_each_connector, ctx);
}

static void do_config(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	unused(argc);
	unused(argv);
	unused(cmd);

	struct ctx *p = (struct ctx *)ctx;
	print_ocpp_configurations(p->io);
}

static void do_pending(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	unused(argc);
	unused(argv);
	unused(cmd);

	struct ctx *p = (struct ctx *)ctx;
	char buf[32];
	snprintf(buf, sizeof(buf), "%zu", ocpp_count_pending_requests());
	printini(p->io, "pending", buf);
}

static const struct cmd cmds[] = {
	{ "info",    NULL, "ocpp info",    2, 2, do_info },
	{ "config",  NULL, "ocpp config",  2, 2, do_config },
	{ "pending", NULL, "ocpp pending", 2, 2, do_pending },
};

DEFINE_CLI_CMD(ocpp, "Interact with OCPP client module") {
	struct cli const *cli = (struct cli const *)env;
	struct app *app = (struct app *)cli->env;
	struct ctx ctx = { .io = cli->io, .app = app };

	if (process_cmd(cmds, ARRAY_COUNT(cmds), argc, argv, &ctx)
			!= CLI_CMD_SUCCESS) {
		println(cli->io, "usage:");
		for (size_t i = 0; i < ARRAY_COUNT(cmds); i++) {
			print_help(cli->io, &cmds[i], NULL);
		}
	}

	return CLI_CMD_SUCCESS;
}
