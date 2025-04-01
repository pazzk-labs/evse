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

#ifndef CLI_HELPER_H
#define CLI_HELPER_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "libmcu/cli.h"

#if !defined(ARRAY_COUNT)
#define ARRAY_COUNT(x)		(sizeof(x) / sizeof((x)[0]))
#endif

struct cmd;
typedef void (*cmd_handler_t)(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx);

struct cmd {
	const char *name;
	const char *opt;
	const char *usage;
	int argc_min;
	int argc_max;
	cmd_handler_t handler;
};

void println(const struct cli_io *io, const char *str);
void printini(const struct cli_io *io, const char *key, const char *value);
void print_help(const struct cli_io *io,
		const struct cmd *cmd, const char *extra);
void print_usage(const struct cli_io *io,
		const struct cmd *cmd, const char *extra);
cli_cmd_error_t process_cmd(const struct cmd *cmds, size_t n,
		int argc, const char *argv[], void *ctx);

#if defined(__cplusplus)
}
#endif

#endif /* CLI_HELPER_H */
