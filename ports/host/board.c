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

#include <time.h>
#include <stdlib.h>
#include <stdbool.h>

uint32_t board_get_time_since_boot_ms(void)
{
	struct timespec spec;
	clock_gettime(CLOCK_MONOTONIC, &spec);
	return (uint32_t)(spec.tv_sec * 1000LL + spec.tv_nsec / 1000000LL);
}

uint64_t board_get_time_since_boot_us(void)
{
	struct timespec spec;
	clock_gettime(CLOCK_MONOTONIC, &spec);
	return (uint64_t)(spec.tv_sec * 1000000LL + spec.tv_nsec / 1000LL);
}

uint32_t board_get_current_stack_watermark(void)
{
	return 0;
}

uint32_t board_get_heap_watermark(void)
{
	return 0;
}

uint32_t board_get_free_heap_bytes(void)
{
	return 0;
}

board_reboot_reason_t board_get_reboot_reason(void)
{
	return BOARD_REBOOT_UNKNOWN;
}

uint32_t board_random(void)
{
	static bool initialized = false;

	if (!initialized) {
		initialized = true;
		srand((unsigned)time(NULL));
	}

	return (uint32_t)rand();
}

void board_reboot(void)
{
}
