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

#ifndef SAFETY_ENTRY_H
#define SAFETY_ENTRY_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <errno.h>

typedef enum {
	SAFETY_STATUS_UNKNOWN,
	SAFETY_STATUS_OK,
	SAFETY_STATUS_ABNORMAL_FREQUENCY,
	SAFETY_STATUS_SAMPLING_ERROR, /**< Not enough samples to determine the
					frequency. */
	SAFETY_STATUS_STALE, /**< The frequency is not updated for a long time.
				This will happen when the power is not normal,
				keeping the line high. */
	SAFETY_STATUS_EMERGENCY_STOP, /**< User emergency stop. */
} safety_entry_status_t;

struct safety_entry;

struct safety_entry_api {
	int (*enable)(struct safety_entry *self);
	int (*disable)(struct safety_entry *self);
	safety_entry_status_t (*check)(struct safety_entry *self);
	int (*get_frequency)(const struct safety_entry *self);
	const char *(*name)(const struct safety_entry *self);
	void (*destroy)(struct safety_entry *self);
};

static inline int safety_entry_enable(struct safety_entry *self) {
	const struct safety_entry_api *api =
		(const struct safety_entry_api *)self;
	return api->enable(self);
}

static inline int safety_entry_disable(struct safety_entry *self) {
	const struct safety_entry_api *api =
		(const struct safety_entry_api *)self;
	return api->disable(self);
}

static inline safety_entry_status_t
safety_entry_check(struct safety_entry *self) {
	const struct safety_entry_api *api =
		(const struct safety_entry_api *)self;
	return api->check(self);
}

static inline int safety_entry_get_frequency(const struct safety_entry *self) {
	const struct safety_entry_api *api =
		(const struct safety_entry_api *)self;
	if (api->get_frequency) {
		return api->get_frequency(self);
	}
	return -ENOTSUP;
}

static inline const char *safety_entry_name(const struct safety_entry *self) {
	const struct safety_entry_api *api =
		(const struct safety_entry_api *)self;
	return api->name(self);
}

static inline void safety_entry_destroy(struct safety_entry *self) {
	const struct safety_entry_api *api =
		(const struct safety_entry_api *)self;
	api->destroy(self);
}

#if defined(__cplusplus)
}
#endif

#endif /* SAFETY_ENTRY_H */
