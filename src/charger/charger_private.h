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

#ifndef CHARGER_PRIVATE_H
#define CHARGER_PRIVATE_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "charger/charger.h"

#include <stdint.h>

#include "libmcu/list.h"
#include "libmcu/ratelim.h"

#define CHARGER_LOG_RATE_CAP		10
#define CHARGER_LOG_RATE_SEC		2

struct charger_api {
	struct connector *(*create_connector)(struct charger *charger,
			const struct connector_param *param);
	void (*destroy_connector)(struct charger *charger, struct list *link);
	int (*step)(struct charger *self, uint32_t *next_period_ms);
};

struct charger {
	struct charger_api api;
	struct charger_param param;
	const char *name;

	struct {
		struct list list;
		uint8_t count;
	} connectors;

	charger_event_cb_t event_cb;
	void *event_cb_ctx;

	struct ratelim log_ratelim; /* rate limiter for logging */
};

struct connector *get_connector(struct charger *charger,
		const char *connector_name);
struct connector *get_connector_free(struct charger *charger);
struct connector *get_connector_by_id(struct charger *charger, const int id);

void register_connector(struct connector *c,
		const struct connector_param *param, struct charger *charger);

#if defined(__cplusplus)
}
#endif

#endif /* CHARGER_PRIVATE_H */
