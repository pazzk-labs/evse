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
#include <stdlib.h>

#include "libmcu/compiler.h"
#include "ocpp/ocpp.h"

static void println(const struct cli_io *io, const char *str)
{
	io->write(str, strlen(str));
	io->write("\n", 1);
}

static void printini(const struct cli_io *io,
		const char *key, const char *value)
{
	io->write(key, strlen(key));
	io->write("=", 1);
	if (value && value[0] != '\0') {
		println(io, value);
	} else {
		println(io, "null");
	}
}

static void print_ocpp_configurations(const struct cli_io *io)
{
#define CFGVAL_STR_MAXLEN		256
	char buf[CFGVAL_STR_MAXLEN];

	for (size_t i = 0; i < ocpp_count_configurations(); i++) {
		const char *key;

		if (!(key = ocpp_get_configuration_keystr_from_index((int)i)) ||
				!ocpp_is_configuration_readable(key)) {
			continue;
		}

		const size_t value_size = ocpp_get_configuration_size(key);
		uint8_t value[value_size];
		bool readonly;

		ocpp_get_configuration(key, value, value_size, &readonly);
		ocpp_stringify_configuration_value(key, buf, sizeof(buf));

		printini(io, key, buf);
	}
}

DEFINE_CLI_CMD(ocpp, "Interact with OCPP client module") {
	unused(argv);
	struct cli const *cli = (struct cli const *)env;

	if (argc == 1) {
		print_ocpp_configurations(cli->io);
	}

	return CLI_CMD_SUCCESS;
}
