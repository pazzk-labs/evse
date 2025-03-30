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

#include "libmcu/timext.h"
#include <stdio.h>

void timeout_set(uint32_t *goal, uint32_t msec)
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	*goal = (uint32_t)(now.tv_sec * 1000 + now.tv_nsec / 1000000L + msec);
}

bool timeout_is_expired(uint32_t goal)
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	struct timespec t = {
		.tv_sec = goal / 1000,
		.tv_nsec = (goal % 1000) * 1000000L,
	};

	if (now.tv_sec > t.tv_sec) {
		return true;
	} else if (now.tv_sec == t.tv_sec && now.tv_nsec >= t.tv_nsec) {
		return true;
	} else {
		return false;
	}
}

void sleep_ms(uint32_t msec)
{
	struct timespec req, rem;

	req.tv_sec = msec / 1000;
	req.tv_nsec = (msec % 1000) * 1000000L;

	if (nanosleep(&req, &rem) == -1) {
		perror("nanosleep");
	}
}
