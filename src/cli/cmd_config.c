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
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "charger/charger.h"
#include "net/util.h"
#include "libmcu/timext.h"
#include "libmcu/pki.h"

struct ctx {
	const struct cli_io *io;
};

static void print_config_version(const struct cli_io *io)
{
	char verstr[16];
	uint32_t version;
	config_get("version", &version, sizeof(version));
	snprintf(verstr, sizeof(verstr), "v%"PRIu32".%"PRIu32".%"PRIu32,
			GET_VERSION_MAJOR(version),
			GET_VERSION_MINOR(version),
			GET_VERSION_PATCH(version));
	printini(io, "config.version", verstr);
}

static void print_charge_mode(const struct cli_io *io)
{
	char mode[CONFIG_CHARGER_MODE_MAXLEN] = { 0, };
	config_get("chg.mode", mode, sizeof(mode));
	printini(io, "mode", mode);
}

static void print_charge_param(const struct cli_io *io)
{
	char buf[32];
	const size_t maxlen = sizeof(buf);
	struct charger_param param;
	charger_default_param(&param);
	config_get("chg.param", &param, sizeof(param));
	snprintf(buf, maxlen, "%dV", param.input_voltage);
	printini(io, "voltage.input", buf);
	snprintf(buf, maxlen, "%"PRIu32"mA", param.max_input_current_mA);
	printini(io, "current.input", buf);
	snprintf(buf, maxlen, "%dHz", param.input_frequency);
	printini(io, "frequency.input", buf);
	snprintf(buf, maxlen, "%"PRIu32"mA ~ %"PRIu32"mA",
			param.min_output_current_mA,
			param.max_output_current_mA);
	printini(io, "current.output", buf);
}

static void print_x509_ca(const struct cli_io *io)
{
	char *buf = (char *)malloc(CONFIG_X509_MAXLEN);
	if (buf) {
		buf[0] = '\0';
		config_get("x509.ca", buf, CONFIG_X509_MAXLEN);
		printini(io, "x509.ca", buf);
		free(buf);
	}
}

static void print_x509_cert(const struct cli_io *io)
{
	char *buf = (char *)malloc(CONFIG_X509_MAXLEN);
	if (buf) {
		buf[0] = '\0';
		config_get("x509.cert", buf, CONFIG_X509_MAXLEN);
		printini(io, "x509.cert", buf);
		free(buf);
	}
}

static void print_ocpp(const struct cli_io *io)
{
	char buf[20+1];

	config_get("ocpp.vendor", buf, sizeof(buf));
	printini(io, "ocpp.vendor", buf);

	config_get("ocpp.model", buf, sizeof(buf));
	printini(io, "ocpp.model", buf);

	uint32_t version;
	config_get("ocpp.version", &version, sizeof(version));
	snprintf(buf, sizeof(buf), "v%"PRIu32".%"PRIu32".%"PRIu32,
			GET_VERSION_MAJOR(version), GET_VERSION_MINOR(version),
			GET_VERSION_PATCH(version));
	printini(io, "ocpp.version", buf);
}

static void do_show(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	unused(argc);
	unused(argv);
	unused(cmd);

	struct ctx *p = (struct ctx *)ctx;

	print_config_version(p->io);
	print_charge_mode(p->io);
	print_charge_param(p->io);
	print_x509_ca(p->io);
	print_x509_cert(p->io);
	print_ocpp(p->io);
}

static void do_reset(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	unused(argc);
	unused(argv);
	unused(cmd);
	struct ctx *p = (struct ctx *)ctx;
	config_reset(NULL);
	println(p->io, "Configuration reset.");
}

static void do_save(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	unused(argc);
	unused(argv);
	unused(cmd);
	struct ctx *p = (struct ctx *)ctx;
	config_save();
	println(p->io, "Configuration saved.");
}

static size_t read_until_eot(const struct cli_io *io,
		char *buf, size_t bufsize, uint32_t timeout_ms)
{
	uint32_t tout;
	size_t i = 0;

	timeout_set(&tout, timeout_ms);

	while (i < bufsize && !timeout_is_expired(tout)) {
		char c;
		if (io->read(&c, 1) == 1) {
			if (c == 0x04/*EOT*/) {
				break;
			} else if (c == '\r') {
				c = '\n';
				if (i > 0 && buf[i-1] == '\n') {
					continue;
				}
			}

			buf[i++] = c;
			io->write(&c, 1);
		}
	}

	buf[i] = '\0';

	return timeout_is_expired(tout)? 0 : i;
}

static void change_ca(const struct cli_io *io)
{
	uint8_t *buf = (uint8_t *)malloc(CONFIG_X509_MAXLEN);

	if (!buf) {
		return;
	}

	size_t len = read_until_eot(io, (char *)buf, CONFIG_X509_MAXLEN, 10000);

	if (len <= 0 || pki_verify_cert(buf, len, buf, len) != 0) {
		println(io, "Failed to verify the certificate");
		goto out_free;
	}

	if (config_set("x509.ca", buf, len+1/*null*/) < 0) {
		println(io, "Failed to save the CA");
	}

out_free:
	free(buf);
}

static void change_cert(const struct cli_io *io)
{
	uint8_t *ca;
	uint8_t *buf;

	if ((buf = (uint8_t *)malloc(CONFIG_X509_MAXLEN)) == NULL) {
		println(io, "Failed to allocate buf");
		return;
	}
	if ((ca = (uint8_t *)malloc(CONFIG_X509_MAXLEN)) == NULL) {
		println(io, "Failed to allocate ca");
		free(buf);
		return;
	}

	size_t len = read_until_eot(io, (char *)buf, CONFIG_X509_MAXLEN, 10000);
	config_get("x509.ca", ca, CONFIG_X509_MAXLEN);
	size_t ca_len = strlen((const char *)ca);

	if (ca_len <= 0 || pki_verify_cert(ca, ca_len, buf, len) != 0) {
		println(io, "Failed to verify the certificate");
		goto out_free;
	}

	if (config_set("x509.cert", buf, len+1/*null*/) < 0) {
		println(io, "Failed to save the certificate");
	}

out_free:
	free(ca);
	free(buf);
}

static void do_read_set(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	unused(argc);

	struct ctx *p = (struct ctx *)ctx;

	if (strcmp(argv[2], "x509.ca") == 0) {
		change_ca(p->io);
	} else if (strcmp(argv[2], "x509.cert") == 0) {
		change_cert(p->io);
	} else {
		print_usage(p->io, cmd, NULL);
	}
}

static void do_set_mode(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	struct ctx *p = (struct ctx *)ctx;

	if (argc != cmd->argc_max) {
		goto out_help;
	}

	char mode[CONFIG_CHARGER_MODE_MAXLEN] = { 0, };

	if (strcmp(argv[3], "free") == 0) {
		strcpy(mode, "free");
	} else if (strcmp(argv[3], "ocpp") == 0) {
		strcpy(mode, "ocpp");
	} else {
		goto out_help;
	}

	if (config_set("chg.mode", mode, strlen(mode)+1/*null*/) == 0) {
		println(p->io, "Reboot to apply the changes after saving");
	}
	return;

out_help:
	print_usage(p->io, cmd, "free | ocpp | hlc");
}

static void do_set_input_voltage(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	struct ctx *p = (struct ctx *)ctx;

	if (argc != cmd->argc_max) {
		goto out_help;
	}

	long voltage = strtol(argv[3], NULL, 10);

	if (voltage < 100 || voltage > 300) {
		goto out_help;
	}

	struct charger_param param;
	charger_default_param(&param);
	config_get("chg.param", &param, sizeof(param));
	param.input_voltage = (int16_t)voltage;
	config_set("chg.param", &param, sizeof(param));
	return;

out_help:
	print_usage(p->io, cmd, "100 ~ 300");
}

static void do_set_input_frequency(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	struct ctx *p = (struct ctx *)ctx;

	if (argc != cmd->argc_max) {
		goto out_help;
	}

	long freq = strtol(argv[3], NULL, 10);

	if (freq < 50 || freq > 60) {
		goto out_help;
	}

	struct charger_param param;
	charger_default_param(&param);
	config_get("chg.param", &param, sizeof(param));
	param.input_frequency = (int16_t)freq;
	config_set("chg.param", &param, sizeof(param));
	return;

out_help:
	print_usage(p->io, cmd, "50 ~ 60");
}

static void do_set_input_current(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	struct ctx *p = (struct ctx *)ctx;

	if (argc != cmd->argc_max) {
		goto out_help;
	}

	long cur = strtol(argv[3], NULL, 10);

	if (cur < 6 || cur > 50) {
		goto out_help;
	}

	struct charger_param param;
	charger_default_param(&param);
	config_get("chg.param", &param, sizeof(param));
	param.max_input_current_mA = (uint32_t)cur * 1000;
	config_set("chg.param", &param, sizeof(param));
	return;

out_help:
	print_usage(p->io, cmd, "6 ~ 50");
}

static void do_set_output_max_current(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	struct ctx *p = (struct ctx *)ctx;

	if (argc != cmd->argc_max) {
		goto out_help;
	}

	struct charger_param param;
	charger_default_param(&param);
	config_get("chg.param", &param, sizeof(param));

	long cur = strtol(argv[3], NULL, 10);

	if (cur < 6 || cur > 50) {
		goto out_help;
	}

	param.max_output_current_mA = (uint32_t)cur * 1000;
	config_set("chg.param", &param, sizeof(param));
	return;

out_help:
	print_usage(p->io, cmd, "6 ~ 50");
}

static void do_set_output_min_current(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	struct ctx *p = (struct ctx *)ctx;

	if (argc != cmd->argc_max) {
		goto out_help;
	}

	struct charger_param param;
	charger_default_param(&param);
	config_set("chg.param", &param, sizeof(param));

	long cur = strtol(argv[3], NULL, 10);

	if (cur < 6 || cur > 50 ||
			(uint32_t)cur > param.max_output_current_mA / 1000) {
		goto out_help;
	}

	param.min_output_current_mA = (uint32_t)cur * 1000;
	config_set("chg.param", &param, sizeof(param));
	return;

out_help:
	print_usage(p->io, cmd, "6 ~ 50, max. output current");
}

static void do_set_multi_chg_param(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	struct ctx *p = (struct ctx *)ctx;

	if (argc != cmd->argc_max) {
		print_usage(p->io, cmd, "<voltage> <current> <frequency> <min> <max>");
		return;
	}

	do_set_input_voltage(cmd, 4, argv, ctx);
	do_set_input_current(cmd, 4, &argv[1], ctx);
	do_set_input_frequency(cmd, 4, &argv[2], ctx);
	do_set_output_max_current(cmd, 4, &argv[3], ctx);
	do_set_output_min_current(cmd, 4, &argv[4], ctx);
}

static void do_set_mac(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	struct ctx *p = (struct ctx *)ctx;

	if (argc != cmd->argc_max) {
		goto out_help;
	}

	uint8_t mac[MAC_ADDR_LEN];
	if (!net_get_mac_from_str(argv[3], mac)) {
		goto out_help;
	}

	config_set("net.mac", mac, sizeof(mac));
	return;

out_help:
	print_usage(p->io, cmd, "01:23:45:67:89:ab");
}

static void do_set_id(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	struct ctx *p = (struct ctx *)ctx;

	if (argc != cmd->argc_max) {
		goto out_help;
	}

	char evse_id[CONFIG_DEVICE_NAME_MAXLEN] = { 0, };

	strncpy(evse_id, argv[3], sizeof(evse_id) - 1);

	config_set("device.name", evse_id, strlen(evse_id)+1/*null*/);
	return;

out_help:
	print_usage(p->io, cmd, "<evse_id>");
}

static void do_reset_id(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	unused(cmd);
	unused(argc);
	unused(argv);
	unused(ctx);

	config_reset("device.name");
}

static const struct cmd cmds[] = {
	{ "show",  NULL,               "config show",  2, 2, do_show },
	{ "save",  NULL,               "config save",  2, 2, do_save },
	{ "set",   "x509.ca",          "config set",   3, 3, do_read_set },
	{ "set",   "x509.cert",        "config set",   3, 3, do_read_set },
	{ "set",   "mac",              "config set",   3, 4, do_set_mac },
	{ "set",   "chg.mode",         "config set",   3, 4, do_set_mode },
	{ "set",   "chg.vol",          "config set",   3, 4, do_set_input_voltage },
	{ "set",   "chg.freq",         "config set",   3, 4, do_set_input_frequency },
	{ "set",   "chg.input_curr",   "config set",   3, 4, do_set_input_current },
	{ "set",   "chg.max_out_curr", "config set",   3, 4, do_set_output_max_current },
	{ "set",   "chg.min_out_curr", "config set",   3, 4, do_set_output_min_current },
	{ "set",   "chg.param",        "config set",   3, 8, do_set_multi_chg_param },
	{ "set",   "chg.id",           "config set",   3, 4, do_set_id },
	{ "reset", "all",              "config reset", 3, 3, do_reset },
	{ "reset", "chg.id",           "config reset", 3, 3, do_reset_id },
};

DEFINE_CLI_CMD(config, "View or modify system configuration") {
	const struct cli *cli = (struct cli const *)env;
	struct ctx ctx = { .io = cli->io };

	if (process_cmd(cmds, ARRAY_COUNT(cmds), argc, argv, &ctx)
			!= CLI_CMD_SUCCESS) {
		println(cli->io, "usage:");
		for (size_t i = 0; i < ARRAY_COUNT(cmds); i++) {
			print_help(cli->io, &cmds[i], NULL);
		}
	}

	return CLI_CMD_SUCCESS;
}
