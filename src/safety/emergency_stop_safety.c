/*
 * This file is part of the Pazzk project <https://pazzk.net/>.
 * Copyright (c) 2025 Pazzk <team@pazzk.net>.
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

#include "emergency_stop_safety.h"

#include <stdbool.h>

#include "libmcu/metrics.h"
#include "libmcu/compiler.h"

#include "usrinp.h"

struct safety_entry {
	struct safety_entry_api api;
	const char *name;
	bool emergency_stop_pressed;
};

static void on_emergency_stop(usrinp_event_t event, void *ctx)
{
	struct safety_entry *self = (struct safety_entry *)ctx;

	switch (event) {
	case USRINP_EVENT_CONNECTED:
		self->emergency_stop_pressed = true;
		metrics_increase(EmergencyStopCount);
		warn("Emergency stop button pressed");
		break;
	case USRINP_EVENT_DISCONNECTED:
		self->emergency_stop_pressed = false;
		metrics_increase(EmergencyReleaseCount);
		info("Emergency stop button released");
		break;
	default:
		break;
	}
}

static safety_entry_status_t check(struct safety_entry *self)
{
	return self->emergency_stop_pressed?
		SAFETY_STATUS_EMERGENCY_STOP : SAFETY_STATUS_OK;
}

static int get_frequency(const struct safety_entry *self)
{
	unused(self);
	return -ENOTSUP;
}

static const char *get_name(const struct safety_entry *self)
{
	return self->name;
}

static int enable(struct safety_entry *self)
{
	usrinp_register_event_cb(USRINP_EMERGENCY_STOP,
			on_emergency_stop, self);
	return 0;
}

static int disable(struct safety_entry *self)
{
	unused(self);
	return -ENOTSUP;
}

static void destroy(struct safety_entry *self)
{
	unused(self);
}

struct safety_entry *emergency_stop_safety_create(const char *name)
{
	static struct safety_entry emergency = {
		.api = {
			.enable = enable,
			.disable = disable,
			.check = check,
			.get_frequency = get_frequency,
			.name = get_name,
			.destroy = destroy,
		},
	};

	emergency.name = name;

	return &emergency;
}
