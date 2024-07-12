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

static void print_all_keys(const struct cli_io *io)
{
	for (int i = 0; i < CONFIG_KEY_MAX; i++) {
		const char *key = config_get_keystr(i);
		if (key) {
			io->write(key, strlen(key));
			io->write("\r\n", 2);
		}
	}
}

DEFINE_CLI_CMD(config, "Configurations") {
	const struct cli *cli = (struct cli const *)env;

	if (argc == 2) {
		if (strcmp(argv[1], "reset") == 0) {
		}
		return CLI_CMD_SUCCESS;
	} else if (argc != 1) {
		return CLI_CMD_INVALID_PARAM;
	}

	print_all_keys(cli->io);

	return CLI_CMD_SUCCESS;
}
