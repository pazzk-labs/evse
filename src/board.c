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

#include "libmcu/board.h"
#include "config.h"
#include <stdio.h>

const char *board_get_serial_number_string(void)
{
	static char sn[CONFIG_DEVICE_ID_MAXLEN];

	if (sn[0] == '\0') {
		config_get("device.id", sn, sizeof(sn));
	}

	return sn;
}

const char *board_get_version_string(void)
{
	static char version[16];

	if (version[0] == '\0') {
		snprintf(version, sizeof(version), "%d.%d.%d",
				GET_VERSION_MAJOR(VERSION),
				GET_VERSION_MINOR(VERSION),
				GET_VERSION_PATCH(VERSION));
	}

	return version;
}

const char *board_name(void)
{
	static char name[CONFIG_DEVICE_NAME_MAXLEN];

	if (name[0] == '\0') {
		config_get("device.name", name, sizeof(name));
	}

	return name;
}
