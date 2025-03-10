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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "net/netmgr.h"
#include "net/util.h"
#include "config.h"

#if !defined(ARRAY_COUNT)
#define ARRAY_COUNT(x)		(sizeof(x) / sizeof((x)[0]))
#endif

static void println(const struct cli_io *io, const char *str)
{
	io->write(str, strlen(str));
	io->write("\n", 1);
}

static void print_state(const struct cli_io *io)
{
	const netmgr_state_t state = netmgr_state();
	const char *str = NULL;

	switch (state) {
	case NETMGR_STATE_CONNECTED:
		str = "Connected.";
		break;
	case NETMGR_STATE_DISCONNECTED:
		str = "Disconnected.";
		break;
	case NETMGR_STATE_OFF:
		str = "Down.";
		break;
	case NETMGR_STATE_EXHAUSTED:
		str = "Exhausted.";
		break;
	default:
		str = "No interface registered.";
		break;
	}

	println(io, str);
}

static void print_url(const struct cli_io *io)
{
#if !defined(URL_MAXLEN)
#define URL_MAXLEN	256
#endif
	char buf[URL_MAXLEN] = { 0, };
	config_get("net.server.url", buf, sizeof(buf));
	println(io, buf);
}

static void print_mac(const struct cli_io *io)
{
	uint8_t mac[MAC_ADDR_LEN] = { 0, };
	char mac_str[18] = { 0, };
	config_get("net.mac", mac, sizeof(mac));
	snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	println(io, mac_str);
}

static void print_health_check_interval(const struct cli_io *io)
{
	uint32_t health_interval = 0;
	config_get("net.health", &health_interval,
			sizeof(health_interval));
	char health_interval_str[12] = { 0, };
	snprintf(health_interval_str, sizeof(health_interval_str), "%u",
			health_interval);
	println(io, health_interval_str);
}

static void print_server_id(const struct cli_io *io)
{
	char id[64] = {0};
	config_get("net.server.id", id, sizeof(id));
	println(io, id);
}

static void print_websocket_ping_interval(const struct cli_io *io)
{
	uint32_t ws_ping_interval = 0;
	config_get("net.server.ping", &ws_ping_interval,
			sizeof(ws_ping_interval));
	char ws_ping_interval_str[12] = { 0, };
	snprintf(ws_ping_interval_str, sizeof(ws_ping_interval_str), "%u",
			ws_ping_interval);
	println(io, ws_ping_interval_str);
}

static void print_all(const struct cli_io *io)
{
	println(io, "[Status]");
	print_state(io);

	println(io, "[Server URL]");
	print_url(io);

	println(io, "[MAC Address]");
	print_mac(io);

	println(io, "[Health Check Interval]");
	print_health_check_interval(io);

	println(io, "[Server ID]");
	print_server_id(io);

	println(io, "[WebSocket Ping Interval]");
	print_websocket_ping_interval(io);
}

typedef bool (*check_fn_t)(const struct cli_io *io,
		const char *key, const char *value);
typedef bool (*cmd_handler_t)(const struct cli_io *io, check_fn_t check,
		const char *key, const char *value, bool is_number);

static bool is_url_valid(const struct cli_io *io, const char *key,
		const char *value)
{
	if (net_get_protocol_from_url(value) == NET_PROTO_UNKNOWN) {
		println(io, "Unknown protocol.");
		return false;
	}
	return true;
}

static bool is_ip_valid(const struct cli_io *io, const char *key,
		const char *value)
{
	return net_is_ipv4_str_valid(value);
}

static bool is_mac_valid(const struct cli_io *io, const char *key,
		const char *value)
{
	uint8_t mac[MAC_ADDR_LEN];
	return net_get_mac_from_str(value, mac);
}

static bool is_health_interval_valid(const struct cli_io *io,
		const char *key, const char *value)
{
	uint32_t intval = (uint32_t)strtol(value, NULL, 10);
	if (intval != 0 && intval < 1000) {
		println(io, "must be 0 or greater than 1s.");
		return false;
	}
	return true;
}

static bool is_ping_interval_valid(const struct cli_io *io,
		const char *key, const char *value)
{
	uint32_t intval = (uint32_t)strtol(value, NULL, 10);
	if (intval != 0 && intval < 60) {
		println(io, "must be 0 or greater than 60s.");
		return false;
	}
	return true;
}

static bool do_ping(const struct cli_io *io, check_fn_t check,
		const char *key, const char *value, bool is_number)
{
	char buf[64] = { 0, };

	if (check && !(*check)(io, key, value)) {
		return false;
	}

	const int result = netmgr_ping(value, 1000);
	if (result < 0) {
		sprintf(buf, "Ping to %s is not reachable.", value);
	} else {
		sprintf(buf, "Ping to %s: %dms", value, result);
	}

	println(io, buf);
	return true;
}

static bool enable(const struct cli_io *io, check_fn_t check,
		const char *key, const char *value, bool is_number)
{
	if (strcmp(value, "enable") == 0) {
		netmgr_enable();
	} else if (strcmp(value, "disable") == 0) {
		netmgr_disable();
	}

	println(io, value);
	return true;
}

static bool update_cfg(const struct cli_io *io, check_fn_t check,
		const char *key, const char *value, bool is_number)
{
	if (check && !(*check)(io, key, value)) {
		return false;
	}

	if (is_number) {
		uint32_t intval = (uint32_t)strtol(value, NULL, 10);
		config_set(key, &intval, sizeof(intval));
	} else {
		if (strcmp(key, "net.mac") == 0) {
			uint8_t mac[MAC_ADDR_LEN];
			net_get_mac_from_str(value, mac);
			config_set(key, mac, sizeof(mac));
		} else {
			config_set(key, value, strlen(value));
		}
	}

	println(io, "Updated.");
	return true;
}

static struct {
	int argc;
	const char *cmd;
	cmd_handler_t run;
	check_fn_t check;
	const char *key;
	bool is_number;
} cmds[] = {
	{ .argc = 2, .cmd = "enable", .run = enable },
	{ .argc = 2, .cmd = "disable", .run = enable },
	{ .argc = 3, .cmd = "mac", .run = update_cfg, .check = is_mac_valid,
		.key = "net.mac", },
	{ .argc = 3, .cmd = "health", .run = update_cfg,
		.check = is_health_interval_valid,
		.key = "net.health", .is_number = true },
	{ .argc = 3, .cmd = "ping", .run = do_ping, .check = is_ip_valid },
	{ .argc = 3, .cmd = "url", .run = update_cfg,
		.check = is_url_valid, .key = "net.server.url" },
	{ .argc = 3, .cmd = "id", .run = update_cfg, .key = "net.server.id" },
	{ .argc = 3, .cmd = "pw", .run = update_cfg, .key = "net.server.pass" },
	{ .argc = 3, .cmd = "ws.ping", .run = update_cfg,
		.check = is_ping_interval_valid,
		.key = "net.server.ping", .is_number = true },
};

DEFINE_CLI_CMD(net, "Network commands") {
	struct cli const *cli = (struct cli const *)env;

	if (argc == 1) {
		print_all(cli->io);
		return CLI_CMD_SUCCESS;
	}

	for (size_t i = 0; i < ARRAY_COUNT(cmds); i++) {
		if (argc == cmds[i].argc &&
				strcmp(argv[1], cmds[i].cmd) == 0) {
			if (cmds[i].run(cli->io, cmds[i].check, cmds[i].key,
					argv[argc-1], cmds[i].is_number)) {
				return CLI_CMD_SUCCESS;
			}
		}
	}

	return CLI_CMD_INVALID_PARAM;
}
