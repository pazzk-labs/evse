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

void println(const struct cli_io *io, const char *str)
{
	io->write(str, strlen(str));
	io->write("\n", 1);
}

void printini(const struct cli_io *io, const char *key, const char *value)
{
	io->write(key, strlen(key));
	io->write("=", 1);
	if (value && value[0] != '\0') {
		println(io, value);
	} else {
		println(io, "null");
	}
}

void print_help(const struct cli_io *io,
		const struct cmd *cmd, const char *extra)
{
	io->write(cmd->usage, strlen(cmd->usage));

	if (cmd->opt) {
		io->write(" ", 1);
		io->write(cmd->opt, strlen(cmd->opt));
	}

	if (extra) {
		io->write(" ", 1);
		io->write(extra, strlen(extra));
	}

	io->write("\n", 1);
}

void print_usage(const struct cli_io *io,
		const struct cmd *cmd, const char *extra)
{
	io->write("usage: ", 7);
	print_help(io, cmd, extra);
}

cli_cmd_error_t process_cmd(const struct cmd *cmds, size_t n,
		int argc, const char *argv[], void *ctx)
{
	for (size_t i = 0; i < n; i++) {
		if (argc >= cmds[i].argc_min && argc <= cmds[i].argc_max &&
				(!cmds[i].name ||
					strcmp(argv[1], cmds[i].name) == 0) &&
				(!cmds[i].opt ||
					strcmp(argv[2], cmds[i].opt) == 0)) {
			(*cmds[i].handler)(&cmds[i], argc, argv, ctx);
			return CLI_CMD_SUCCESS;
		}
	}

	return CLI_CMD_INVALID_PARAM;
}
