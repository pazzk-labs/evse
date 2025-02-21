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
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "charger/charger.h"

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

static void print_charge_param(const struct cli_io *io)
{
	char buf[64];
	struct charger_param param;
	charger_default_param(&param);
	config_read(CONFIG_KEY_CHARGER_PARAM, &param, sizeof(param));
	snprintf(buf, sizeof(buf), "Input voltage: %dV", param.input_voltage);
	println(io, buf);
	snprintf(buf, sizeof(buf), "Input current: %umA", param.max_input_current_mA);
	println(io, buf);
	snprintf(buf, sizeof(buf), "Input frequency: %dHz", param.input_frequency);
	println(io, buf);
	snprintf(buf, sizeof(buf), "Output current: %umA ~ %umA",
			param.min_output_current_mA,
			param.max_output_current_mA);
	println(io, buf);
}

static void change_charge_mode(const struct cli_io *io, const char *str)
{
	char mode[16];

	if (strcmp(str, "free") == 0) {
		strcpy(mode, "free");
	} else if (strcmp(str, "ocpp") == 0) {
		strcpy(mode, "ocpp");
	} else {
		println(io, "invalid mode");
		return;
	}

	if (config_save(CONFIG_KEY_CHARGER_MODE, mode, strlen(mode)) == 0) {
		println(io, "Reboot to apply the changes.");
	}
}

static void change_input_voltage(const struct cli_io *io, const char *str)
{
	long voltage = strtol(str, NULL, 10);

	if (voltage < 100 || voltage > 300) {
		println(io, "invalid voltage");
		return;
	}

	struct charger_param param;
	charger_default_param(&param);
	config_read(CONFIG_KEY_CHARGER_PARAM, &param, sizeof(param));
	param.input_voltage = (int16_t)voltage;
	config_save(CONFIG_KEY_CHARGER_PARAM, &param, sizeof(param));
}

static void change_input_frequency(const struct cli_io *io, const char *str)
{
	long freq = strtol(str, NULL, 10);

	if (freq < 50 || freq > 60) {
		println(io, "invalid frequency");
		return;
	}

	struct charger_param param;
	charger_default_param(&param);
	config_read(CONFIG_KEY_CHARGER_PARAM, &param, sizeof(param));
	param.input_frequency = (int16_t)freq;
	config_save(CONFIG_KEY_CHARGER_PARAM, &param, sizeof(param));
}

static void change_input_current(const struct cli_io *io, const char *str)
{
	long cur = strtol(str, NULL, 10);

	if (cur < 6 || cur > 50) {
		println(io, "invalid input current");
		return;
	}

	struct charger_param param;
	charger_default_param(&param);
	config_read(CONFIG_KEY_CHARGER_PARAM, &param, sizeof(param));
	param.max_input_current_mA = (uint32_t)cur * 1000;
	config_save(CONFIG_KEY_CHARGER_PARAM, &param, sizeof(param));
}

static void change_output_max_current(const struct cli_io *io, const char *str)
{
	struct charger_param param;
	charger_default_param(&param);
	config_read(CONFIG_KEY_CHARGER_PARAM, &param, sizeof(param));

	long cur = strtol(str, NULL, 10);

	if (cur < 6 || cur > 50) {
		println(io, "invalid output max. current");
		return;
	}

	param.max_output_current_mA = (uint32_t)cur * 1000;
	config_save(CONFIG_KEY_CHARGER_PARAM, &param, sizeof(param));
}

static void change_output_min_current(const struct cli_io *io, const char *str)
{
	struct charger_param param;
	charger_default_param(&param);
	config_read(CONFIG_KEY_CHARGER_PARAM, &param, sizeof(param));

	long cur = strtol(str, NULL, 10);

	if (cur < 6 || cur > 50 || cur > param.max_output_current_mA / 1000) {
		println(io, "invalid output min. current");
		return;
	}

	param.min_output_current_mA = (uint32_t)cur * 1000;
	config_save(CONFIG_KEY_CHARGER_PARAM, &param, sizeof(param));
}

static void set_config(const struct cli_io *io,
		const char *key, const char *value)
{
	if (strcmp(key, "chg/mode") == 0) {
		change_charge_mode(io, value);
	} else if (strcmp(key, "mac") == 0) {
	} else if (strcmp(key, "chg/vol") == 0) {
		change_input_voltage(io, value);
	} else if (strcmp(key, "chg/freq") == 0) {
		change_input_frequency(io, value);
	} else if (strcmp(key, "chg/input_curr") == 0) {
		change_input_current(io, value);
	} else if (strcmp(key, "chg/max_out_curr") == 0) {
		change_output_max_current(io, value);
	} else if (strcmp(key, "chg/min_out_curr") == 0) {
		change_output_min_current(io, value);
	}
}

static void set_config_multi_param(const struct cli_io *io,
		int argc, const char *argv[])
{
	if (argc == 8 && strcmp(argv[2], "chg/param") == 0) {
		change_input_voltage(io, argv[3]);
		change_input_current(io, argv[4]);
		change_input_frequency(io, argv[5]);
		change_output_max_current(io, argv[7]);
		change_output_min_current(io, argv[6]);
	}
}

DEFINE_CLI_CMD(config, "Configurations") {
	const struct cli *cli = (struct cli const *)env;

	if (argc >= 2) {
		if (argc == 2 && strcmp(argv[1], "reset") == 0) {
		} else if (argc == 4 && strcmp(argv[1], "set") == 0) {
			set_config(cli->io, argv[2], argv[3]);
		} else if (argc > 4 && strcmp(argv[1], "set") == 0) {
			set_config_multi_param(cli->io, argc, argv);
		}
		return CLI_CMD_SUCCESS;
	} else if (argc != 1) {
		return CLI_CMD_INVALID_PARAM;
	}

	println(cli->io, "[Charge mode]");
	print_charge_mode(cli->io);
	println(cli->io, "[Charge parameters]");
	print_charge_param(cli->io);
	println(cli->io, "[Other configurations]");
	print_all_keys(cli->io);

	return CLI_CMD_SUCCESS;
}
