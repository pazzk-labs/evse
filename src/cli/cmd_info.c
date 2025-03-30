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

#include "libmcu/cli.h"

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <inttypes.h>

#include "libmcu/board.h"
#include "fs/fs.h"
#include "uptime.h"
#include "app.h"

typedef enum {
	CMD_OPT_NONE			= 0x00,
	CMD_OPT_VERSION			= 0x01,
	CMD_OPT_BUILD_DATE		= 0x02,
	CMD_OPT_SERIAL_NUMBER		= 0x04,
	CMD_OPT_CPULOAD			= 0x08,
	CMD_OPT_ALL			= (
			CMD_OPT_VERSION |
			CMD_OPT_BUILD_DATE |
			CMD_OPT_CPULOAD |
			CMD_OPT_SERIAL_NUMBER),
} cmd_opt_t;

static void printini(const struct cli_io *io,
		const char *key, const char *value)
{
	io->write(key, strlen(key));
	io->write("=", 1);
	io->write(value, strlen(value));
	io->write("\n", 1);
}

static cmd_opt_t get_command_option(int argc, const char *opt)
{
	if (argc == 1) {
		return CMD_OPT_ALL;
	} else if (opt == NULL) {
		return CMD_OPT_NONE;
	} else if (strcmp(opt, "version") == 0) {
		return CMD_OPT_VERSION;
	} else if (strcmp(opt, "sn") == 0) {
		return CMD_OPT_SERIAL_NUMBER;
	} else if (strcmp(opt, "build") == 0) {
		return CMD_OPT_BUILD_DATE;
	}

	return CMD_OPT_ALL;
}

static void print_version(struct cli_io const *io)
{
	printini(io, "fw-ver", board_get_version_string());
}

static void print_sn(struct cli_io const *io)
{
	printini(io, "SN", board_get_serial_number_string());
}

static void print_build_date(struct cli_io const *io)
{
	printini(io, "build-date", board_get_build_date_string());
}

static void print_cpuload(struct cli_io const *io)
{
	char buf[64];
	snprintf(buf, sizeof(buf)-1, "%u%% %u%%, %u%% %u%%, %u%% %u%%",
			board_cpuload(0, 1), board_cpuload(1, 1),
			board_cpuload(0, 60), board_cpuload(1, 60),
			board_cpuload(0, 5*60), board_cpuload(1, 5*60));
	printini(io, "cpuload", buf);
}

static void print_board_time(struct cli_io const *io)
{
	char buf[32];
	snprintf(buf, sizeof(buf)-1, "%"PRIu32, board_get_time_since_boot_ms());
	printini(io, "monotonic-time", buf);

#if defined(__APPLE__)
	snprintf(buf, sizeof(buf)-1, "%"PRId64, (uint64_t)time(NULL));
#else
	snprintf(buf, sizeof(buf)-1, "%"PRId64, time(NULL));
#endif
	printini(io, "walltime", buf);
}

static void print_device_info(struct cli_io const *io)
{
	printini(io, "reboot-reason", board_get_reboot_reason_string(
			board_get_reboot_reason()));

	char buf[32];
	snprintf(buf, sizeof(buf)-1, "%"PRIu32"/%"PRIu32,
			board_get_heap_watermark(), board_get_free_heap_bytes());
	printini(io, "heap", buf);

	snprintf(buf, sizeof(buf)-1, "%"PRIu32,
			board_get_current_stack_watermark());
	printini(io, "stack", buf);
}

static void print_uptime(struct cli_io const *io)
{
	char buf[32];
	const time_t sec = uptime_get();
	const uint32_t hours = (const uint32_t)(sec / 3600);
	const uint32_t days = hours / 24;

	snprintf(buf, sizeof(buf)-1,
			"%"PRIu32" hours (%"PRIu32" days)", hours, days);
	printini(io, "Uptime", buf);
}

static void print_fs_info(struct cli_io const *io, struct app *app)
{
	size_t used_bytes;
	size_t total_bytes;
	fs_usage(app->fs, &used_bytes, &total_bytes);

	char buf[128];
	snprintf(buf, sizeof(buf)-1, "%zu/%zu bytes (%zu%% used)",
			used_bytes, total_bytes,
			used_bytes*100/total_bytes);
	printini(io, "Filesystem", buf);
}

DEFINE_CLI_CMD(info, "Display device info") {
	struct cli *cli = (struct cli *)env;
	struct app *app = (struct app *)cli->env;

	if (argc > 2) {
		return CLI_CMD_INVALID_PARAM;
	}

	cmd_opt_t options = get_command_option(argc, argv? argv[1] : NULL);

	if (options & CMD_OPT_VERSION) {
		print_version(cli->io);
	}
	if (options & CMD_OPT_SERIAL_NUMBER) {
		print_sn(cli->io);
	}
	if (options & CMD_OPT_BUILD_DATE) {
		print_build_date(cli->io);
	}
	if (options & CMD_OPT_CPULOAD) {
		print_cpuload(cli->io);
	}

	print_board_time(cli->io);
	print_device_info(cli->io);
	print_uptime(cli->io);
	print_fs_info(cli->io, app);

	return CLI_CMD_SUCCESS;
}
