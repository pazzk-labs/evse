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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "libmcu/compiler.h"
#include "libmcu/wifi.h"
#include "net/wifi_ap_info.h"
#include "config.h"
#include "app.h"

struct ctx {
	const struct cli_io *io;
	struct app *app;
};

static void print_wifi_info(const struct cli_io *io, struct wifi *wifi)
{
	struct wifi_iface_info info;
	char buf[32];

	wifi_get_status(wifi, &info);
	snprintf(buf, sizeof(buf), "%s", (char *)info.ssid);
	printini(io, "wifi.ssid", buf);
	snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
			info.mac[0], info.mac[1], info.mac[2],
			info.mac[3], info.mac[4], info.mac[5]);
	printini(io, "wifi.mac", buf);
	snprintf(buf, sizeof(buf), "%d", info.rssi);
	printini(io, "wifi.rssi", buf);
	snprintf(buf, sizeof(buf), "%d", info.channel);
	printini(io, "wifi.channel", buf);
	snprintf(buf, sizeof(buf), "%d.%d.%d.%d",
			info.ip_info.v4.ip.addr[0], info.ip_info.v4.ip.addr[1],
			info.ip_info.v4.ip.addr[2], info.ip_info.v4.ip.addr[3]);
	printini(io, "wifi.ip", buf);
	snprintf(buf, sizeof(buf), "%d.%d.%d.%d",
			info.ip_info.v4.netmask.addr[0],
			info.ip_info.v4.netmask.addr[1],
			info.ip_info.v4.netmask.addr[2],
			info.ip_info.v4.netmask.addr[3]);
	printini(io, "wifi.netmask", buf);
	snprintf(buf, sizeof(buf), "%d.%d.%d.%d",
			info.ip_info.v4.gateway.addr[0],
			info.ip_info.v4.gateway.addr[1],
			info.ip_info.v4.gateway.addr[2],
			info.ip_info.v4.gateway.addr[3]);
	printini(io, "wifi.gateway", buf);
}

static void do_info(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	unused(argc);
	unused(argv);
	unused(cmd);

	struct ctx *p = (struct ctx *)ctx;
	uint8_t opt;

	config_get("net.opt", &opt, sizeof(opt));

	printini(p->io, "wifi.status", opt & CONFIG_WIFI_ENABLED?
			"enabled" : "disabled");

	if (opt & CONFIG_WIFI_ENABLED) {
		print_wifi_info(p->io, p->app->wifi);
	}
}

static void do_enable(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	unused(cmd);
	unused(argc);
	unused(argv);
	unused(ctx);

	uint8_t opt = CONFIG_WIFI_ENABLED;
	config_set("net.opt", &opt, sizeof(opt));
	config_save();
}

static void do_disable(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	unused(cmd);
	unused(argc);
	unused(argv);
	unused(ctx);

	uint8_t opt = CONFIG_ETHERNET_ENABLED;
	config_set("net.opt", &opt, sizeof(opt));
	config_save();
}

static void do_add(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	struct ctx *p = (struct ctx *)ctx;

	if (argc != 5) {
		goto out_help;
	}
	const char *ssid = argv[3];
	const char *pass = argv[4];

	struct wifi_ap_info ap_info = {
		.security = WIFI_SEC_TYPE_PSK,
	};
	memcpy(ap_info.ssid, ssid, strlen(ssid));
	memcpy(ap_info.pass, pass, strlen(pass));

	if (wifi_ap_info_add(p->app->fs, &ap_info) != 0) {
		println(p->io, "Failed to add WiFi AP info");
	}
	return;

out_help:
	print_usage(p->io, cmd, "<ssid> <password>");
}

static void do_list(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	unused(cmd);
	unused(argc);
	unused(argv);

	struct ctx *p = (struct ctx *)ctx;
	struct wifi_ap_info *ap_list =
		malloc(sizeof(*ap_list) * WIFI_AP_INFO_MAX_COUNT);
	if (!ap_list) {
		println(p->io, "Failed to allocate memory for AP list");
		return;
	}

	int cnt = wifi_ap_info_get_all(p->app->fs,
			ap_list, WIFI_AP_INFO_MAX_COUNT);

	for (int i = 0; i < cnt; i++) {
		struct wifi_ap_info *ap = &ap_list[i];
		printini(p->io, "wifi.ap", ap->ssid);
	}

	free(ap_list);
}

static void do_del(const struct cmd *cmd,
		int argc, const char *argv[], void *ctx)
{
	struct ctx *p = (struct ctx *)ctx;

	if (argc == 3 && strcmp(argv[2], "all") == 0) {
		wifi_ap_info_del_all(p->app->fs);
	} else {
		goto out_help;
	}
	return;

out_help:
	print_usage(p->io, cmd, "<ssid>");
}

static const struct cmd cmds[] = {
	{ "info",    NULL,  "wifi info",    2, 2, do_info },
	{ "enable",  NULL,  "wifi enable",  2, 2, do_enable },
	{ "disable", NULL,  "wifi disable", 2, 2, do_disable },
	{ "list",    NULL,  "wifi list",    2, 2, do_list },
	{ "add",     "ap",  "wifi add",     3, 5, do_add },
	{ "del",     "all", "wifi del",     3, 3, do_del },
#if 0
	{ "del",     "ap",  "wifi del",     3, 4, do_del },
	{ "scan",    NULL,  "wifi scan",    2, 2, NULL },
#endif
};

DEFINE_CLI_CMD(wifi, "Manage WiFi settings") {
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
