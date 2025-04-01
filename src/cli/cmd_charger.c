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

#include "helper.h"

#include <string.h>
#include <stdio.h>

#include "libmcu/compiler.h"
#include "charger/charger.h"
#include "charger/connector.h"
#include "app.h"

struct ctx {
	const struct cli_io *io;
	struct app *app;
};

static void on_each_connector(struct connector *c, void *ctx)
{
	struct ctx *p = (struct ctx *)ctx;
	const char *name = connector_name(c);
	const char *state = connector_stringify_state(connector_state(c));
	char buf[64];
	snprintf(buf, sizeof(buf), "connector.%s.state", name);
	printini(p->io, buf, state);
}

static void do_show(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	unused(argc);
	unused(argv);
	unused(cmd);

	struct ctx *p = (struct ctx *)ctx;

	const int n = charger_count_connectors(p->app->charger);
	char buf[16];
	snprintf(buf, sizeof(buf), "%d", n);
	printini(p->io, "charger.connectors", buf);

	charger_iterate_connectors(p->app->charger, on_each_connector, ctx);
}

#if defined(HOST_BUILD)
static void do_pilot_set(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	struct ctx *p = (struct ctx *)ctx;

	if (argc != cmd->argc_max || strlen(argv[3]) != 1) {
		goto out_help;
	}

	if (argv[3][0] == 'A' || argv[3][0] == 'a') {
		pilot_set_status(p->app->pilot, PILOT_STATUS_A);
	} else if (argv[3][0] == 'B' || argv[3][0] == 'b') {
		pilot_set_status(p->app->pilot, PILOT_STATUS_B);
	} else if (argv[3][0] == 'C' || argv[3][0] == 'c') {
		pilot_set_status(p->app->pilot, PILOT_STATUS_C);
	} else if (argv[3][0] == 'D' || argv[3][0] == 'd') {
		pilot_set_status(p->app->pilot, PILOT_STATUS_D);
	} else if (argv[3][0] == 'E' || argv[3][0] == 'e') {
		pilot_set_status(p->app->pilot, PILOT_STATUS_E);
	} else if (argv[3][0] == 'F' || argv[3][0] == 'f') {
		pilot_set_status(p->app->pilot, PILOT_STATUS_F);
	} else {
		goto out_help;
	}
	printini(p->io, "pilot.status", argv[3]);
	return;

out_help:
	print_usage(p->io, cmd, "{A|B|C|D|E|F}");
}
#endif /* HOST_BUILD */

static const struct cmd cmds[] = {
	{ "show",  NULL,    "chg show", 2, 2, do_show },
#if defined(HOST_BUILD)
	{ "set",   "pilot", "chg set",  3, 4, do_pilot_set },
#endif /* HOST_BUILD */
};

DEFINE_CLI_CMD(chg, 0) {
	struct cli const *cli = (struct cli const *)env;
	struct app *app = (struct app *)cli->env;
	struct ctx ctx = { .io = cli->io, .app = app };

	if (argc < 2) {
		do_show(NULL, argc, argv, &ctx);
		return CLI_CMD_SUCCESS;
	}

	if (process_cmd(cmds, ARRAY_COUNT(cmds), argc, argv, &ctx)
			!= CLI_CMD_SUCCESS) {
		println(cli->io, "usage:");
		for (size_t i = 0; i < ARRAY_COUNT(cmds); i++) {
			print_help(cli->io, &cmds[i], NULL);
		}
	}

	return CLI_CMD_SUCCESS;
}
