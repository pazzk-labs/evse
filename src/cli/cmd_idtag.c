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
#include "charger/ocpp_connector.h"
#include "app.h"
#include "logger.h"

struct ctx {
	const struct cli_io *io;
	struct app *app;
	const char *idstr;
};

static void on_occupy_result(struct connector *c,
		ocpp_session_result_t result, void *ctx)
{
	unused(ctx);

	if (result == OCPP_SESSION_RESULT_OK) {
		ocpp_connector_occupy(c);
	}
}

static void on_release_result(struct connector *c,
		ocpp_session_result_t result, void *ctx)
{
	unused(ctx);

	if (result == OCPP_SESSION_RESULT_OK &&
			ocpp_connector_can_user_release(c)) {
		ocpp_connector_release(c, false);
	}
}

static void do_tag(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	unused(argc);
	unused(cmd);

	struct ctx *p = (struct ctx *)ctx;
	const struct cli_io *io = p->io;
	printini(io, "tag", argv[1]);

	if (!charger_supports(p->app->charger, "ocpp")) {
		println(io, "idtag command is not supported");
		return;
	}

	struct connector *c = charger_get_connector_available(p->app->charger);
	if (!c) {
		if (!(c = charger_get_connector_by_id(p->app->charger, 1))) {
			println(io, "No connector found");
			return;
		}
		println(io, "Using the first connector");
	}

	if (ocpp_connector_has_active_authorization(c)) {
		println(io, "Already occupied");
		ocpp_connector_try_release(c, argv[1], strlen(argv[1]),
				on_release_result, ctx);
		return;
	}

	int err = ocpp_connector_try_occupy(c, argv[1], strlen(argv[1]), false,
			on_occupy_result, ctx);
	if (err) {
		println(io, "Failed to occupy connector");
		return;
	}
}

static const struct cmd cmds[] = {
	{ NULL, NULL, "idtag {string}", 2, 2, do_tag },
};

DEFINE_CLI_CMD(idtag, "Set or update the user ID tag") {
	struct cli const *cli = (struct cli const *)env;
	struct ctx ctx = { .io = cli->io, .app = (struct app *)cli->env };

	if (argc > 1) {
		ctx.idstr = argv[1];
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
