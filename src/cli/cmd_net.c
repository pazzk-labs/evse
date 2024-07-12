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
#include "net/netmgr.h"

static void println(const struct cli_io *io, const char *str)
{
	io->write(str, strlen(str));
	io->write("\n", 1);
}

static void do_power(bool down, const struct cli_io *io)
{
	if (down) {
		netmgr_disable();
		println(io, "Disabled.");
	} else {
		netmgr_enable();
		println(io, "Enabled.");
	}
}

static void do_ping(const char *host, const struct cli_io *io)
{
	char buf[64] = { 0, };
	/* FIXME: check if host string is valid. ping input must be an IP
	 * address string. */
	const int result = netmgr_ping(host, 1000);
	if (result < 0) {
		sprintf(buf, "Ping to %s is not reachable.", host);
	} else {
		sprintf(buf, "Ping to %s: %dms", host, result);
	}
	println(io, buf);
}

static void do_print_state(const struct cli_io *io)
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

DEFINE_CLI_CMD(net, "Network commands") {
	struct cli const *cli = (struct cli const *)env;

	if (argc == 1) {
		do_print_state(cli->io);
	} else if (strcmp(argv[1], "enable") == 0) {
		do_power(0, cli->io);
	} else if (strcmp(argv[1], "disable") == 0) {
		do_power(1, cli->io);
	} else if (argc == 3 && strcmp(argv[1], "ping") == 0) {
		do_ping(argv[2], cli->io);
	}

	return CLI_CMD_SUCCESS;
}
