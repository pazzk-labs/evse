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

#include "tmp102.h"

static void *user_context;

int tmp102_read(int16_t *temperature)
{
	uint16_t raw;
	int err = 0;
	int negative = 1;

	if ((err = tmp102_port_read_reg(user_context,
			0x00, (uint8_t *)&raw, sizeof(raw))) < 0) {
		return err;
	}

	raw = (raw >> 8) | ((raw & 0xff) << 8);
	raw >>= 4;

	if (raw & 0x800) {
		raw = 0 - ((raw ^ 0xfff) + 1);
		negative = -1;
	}

	*temperature = (raw * 625 / 1000) * negative;

	return err;
}

int tmp102_init(void *ctx)
{
	user_context = ctx;
	return 0;
}
