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

#include "charger/free.h"

#include <string.h>
#include <stdlib.h>

#include "../charger_private.h"
#include "../connector_private.h"
#include "libmcu/fsm.h"
#include "libmcu/metrics.h"
#include "logger.h"

#define DEFAULT_PROCESSING_INTERVAL_MS		10

struct free_charger {
	struct charger base;
};

struct free_charger_connector {
	struct connector base;
	struct fsm fsm;
};

static void update_metrics(const fsm_state_t state)
{
	const metric_key_t tbl[Sn] = {
		[A] = ChargerStateACount,
		[B] = ChargerStateBCount,
		[C] = ChargerStateCCount,
		[D] = ChargerStateDCount,
		[E] = ChargerStateECount,
		[F] = ChargerStateFCount,
	};
	metrics_increase(tbl[state]);
}

static void on_state_change(struct fsm *fsm,
		fsm_state_t new_state, fsm_state_t prev_state, void *ctx)
{
	struct connector *connector = (struct connector *)ctx;
	connector->time_last_state_change = time(NULL);

	info("connector(%d) state changed: %s to %s at %ld", connector->id,
			stringify_state((connector_state_t)prev_state),
			stringify_state((connector_state_t)new_state),
			connector->time_last_state_change);

	const charger_event_t events =
		get_event_from_state_change((connector_state_t)new_state,
				(connector_state_t)prev_state);

	if (events && connector->charger->event_cb) {
		connector->charger->event_cb(connector->charger, connector,
				events, connector->charger->event_cb_ctx);
	}

	update_metrics(new_state);
}

static int step(struct charger *self, uint32_t *next_period_ms)
{
	struct list *p;

	list_for_each(p, &self->connectors.list) {
		struct free_charger_connector *c = list_entry(p,
				struct free_charger_connector, base.link);
		fsm_step(&c->fsm);
	}

	if (next_period_ms) {
		*next_period_ms = DEFAULT_PROCESSING_INTERVAL_MS;
	}

	return 0;
}

static struct connector *create_connector(struct charger *charger,
			const struct connector_param *param)
{
	struct free_charger_connector *c;

	if (!(c = (struct free_charger_connector *)malloc(sizeof(*c)))) {
		return NULL;
	}

	memset(c, 0, sizeof(*c));

	register_connector(&c->base, param, charger);

	size_t n = 0;
	const struct fsm_item *transitions = get_fsm_table(&n);
	fsm_init(&c->fsm, transitions, n, c);
	fsm_set_state_change_cb(&c->fsm, on_state_change, c);

	debug("connector created(%d): %s", c->base.id, c->base.param.name);

	return &c->base;
}

static void destroy_connector(struct charger *charger, struct list *link)
{
	struct free_charger_connector *c =
		list_entry(link, struct free_charger_connector, base.link);

	if (c) {
		list_del(&c->base.link, &charger->connectors.list);
		free(c);
	}
}

static const struct charger_api *get_api(void)
{
	static struct charger_api api = {
		.create_connector = create_connector,
		.destroy_connector = destroy_connector,
		.step = step,
	};

	return &api;
}

struct charger *free_charger_create(const struct charger_param *param)
{
	static struct free_charger free_charger;
	struct charger *charger = &free_charger.base;

	memset(&free_charger, 0, sizeof(free_charger));

	*charger = (struct charger) {
		.api = *get_api(),
		.name = "free",
	};

	if (charger_init(charger, param) == 0) {
		return charger;
	}

	return NULL;
}

void free_charger_destroy(struct charger *charger)
{
	if (!charger) {
		return;
	}

	charger_deinit(charger);
}
