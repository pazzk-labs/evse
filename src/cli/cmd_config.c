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
#include <inttypes.h>
#include "config.h"

static void println(const struct cli_io *io, const char *str)
{
	io->write(str, strlen(str));
	io->write("\n", 1);
}

static void print_all_keys(const struct cli_io *io)
{
	for (int i = 0; i < CONFIG_KEY_MAX; i++) {
		const char *key = config_get_keystr(i);
		if (key) {
			println(io, key);
		}
	}
}

static void print_charge_mode(const struct cli_io *io)
{
	char mode[16] = { 0, };
	config_read(CONFIG_KEY_CHARGER_MODE, mode, sizeof(mode));
	println(io, mode);
}

static void change_charge_mode(const struct cli_io *io, const char *str)
{
	char mode[16];

	if (strcmp(str, "free") == 0) {
		strcpy(mode, "free");
	} else if (strcmp(str, "ocpp") == 0) {
		strcpy(mode, "ocpp");
	} else {
		println(io, "Invalid mode");
		return;
	}

	if (config_save(CONFIG_KEY_CHARGER_MODE, mode, strlen(mode)) == 0) {
		println(io, "Reboot to apply the changes.");
	}
}

static void set_config(const struct cli_io *io,
		const char *key, const char *value)
{
	if (strcmp(key, "chg") == 0) {
		change_charge_mode(io, value);
	} else if (strcmp(key, "mac") == 0) {
	}
}

DEFINE_CLI_CMD(config, "Configurations") {
	const struct cli *cli = (struct cli const *)env;

	if (argc >= 2) {
		if (argc == 2 && strcmp(argv[1], "reset") == 0) {
		} else if (argc == 4 && strcmp(argv[1], "set") == 0) {
			set_config(cli->io, argv[2], argv[3]);
		}
		return CLI_CMD_SUCCESS;
	} else if (argc != 1) {
		return CLI_CMD_INVALID_PARAM;
	}

	println(cli->io, "Charge mode:");
	print_charge_mode(cli->io);
	println(cli->io, "Other configurations:");
	print_all_keys(cli->io);

	return CLI_CMD_SUCCESS;
}
