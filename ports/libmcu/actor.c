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

#include "libmcu/actor.h"
#include "libmcu/actor_timer.h"
#include "libmcu/metrics.h"
#include "libmcu/board.h"

static uint32_t t0;

void actor_pre_dispatch_hook(const struct actor *actor,
		const struct actor_msg *msg)
{
	unused(actor);
	unused(msg);

	const uint32_t t1 = (uint32_t)board_get_time_since_boot_ms();
	const uint32_t elapsed = t1 - t0;

	metrics_set_if_max(ActorIntervalMax, METRICS_VALUE(elapsed));
	metrics_set_if_min(ActorIntervalMin, METRICS_VALUE(elapsed));

	t0 = t1;
	metrics_increase(ActorDispatchCount);
}

void actor_post_dispatch_hook(const struct actor *actor,
		const struct actor_msg *msg)
{
	unused(actor);
	unused(msg);

	const uint32_t elapsed = (uint32_t)board_get_time_since_boot_ms() - t0;

	metrics_set_if_max(ActorLongestTime, METRICS_VALUE(elapsed));
	metrics_set(ActorQueueCount, actor_len());
	metrics_set(ActorTimerQueueCount, actor_timer_len());
	metrics_set(MainStackHighWatermark,
			board_get_current_stack_watermark());
}
