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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "libmcu/compiler.h"
#include "libmcu/wdt.h"
#include "app.h"

#if !defined(HOST_BUILD)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

struct ctx {
	const struct cli_io *io;
	struct app *app;
};

static void on_each_wdt(struct wdt *wdt, void *ctx)
{
	struct ctx *p = (struct ctx *)ctx;
	char buf[32];

	printini(p->io, "wdt.name", wdt_name(wdt));
	snprintf(buf, sizeof(buf), "%s", wdt_is_enabled(wdt)?
			"enabled" : "disabled");
	printini(p->io, "wdt.status", buf);
	snprintf(buf, sizeof(buf), "%"PRIu32, wdt_get_period(wdt));
	printini(p->io, "wdt.period", buf);
	snprintf(buf, sizeof(buf), "%"PRIu32, wdt_get_time_since_last_feed(wdt));
	printini(p->io, "wdt.time_since_last_feed", buf);
}

static void do_wdt(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	unused(cmd);
	unused(argc);
	unused(argv);

	wdt_foreach(on_each_wdt, ctx);
}

static void do_task(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	unused(cmd);
	unused(argc);
	unused(argv);

	struct ctx *p = (struct ctx *)ctx;
	char *str = (char *)malloc(4096);

	if (!str) {
		println(p->io, "Failed to allocate memory for task list.");
		return;
	}

#if !defined(HOST_BUILD)
	vTaskList(str);
	println(p->io, str);
	vTaskGetRunTimeStats(str);
	println(p->io, str);
#endif

	free(str);
}

static const struct cmd cmds[] = {
	{ "wdt",  NULL, "dbg wdt",  2, 2, do_wdt },
	{ "task", NULL, "dbg task", 2, 2, do_task },
};

DEFINE_CLI_CMD(dbg, "Debug commands") {
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
