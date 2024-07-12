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

#include "charger_private.h"
#include "connector_private.h"
#include <string.h>
#include "logger.h"

struct connector *get_connector(struct charger *charger,
		const char *connector_name)
{
	struct list *p;

	list_for_each(p, &charger->connectors.list) {
		struct connector *c = list_entry(p, struct connector, link);
		if (strcmp(c->param.name, connector_name) == 0) {
			return c;
		}
	}

	return NULL;
}

struct connector *get_connector_free(struct charger *charger)
{
	/* TODO: Implement this function */
	return NULL;
}

struct connector *get_connector_by_id(struct charger *charger, const int id)
{
	struct list *p;

	list_for_each(p, &charger->connectors.list) {
		struct connector *c = list_entry(p, struct connector, link);
		if (c->id == id) {
			return c;
		}
	}

	return NULL;
}

void register_connector(struct connector *c,
		const struct connector_param *param, struct charger *charger)
{
	list_add_tail(&c->link, &charger->connectors.list);

	c->param = *param;
	c->charger = charger;
	c->id = charger->connectors.count + 1;
	c->time_last_state_change = time(NULL);

	debug("connector registered(%d): %s", c->id, c->param.name);
}
