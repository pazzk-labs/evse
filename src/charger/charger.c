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

#include "charger_internal.h"
#include "charger/connector.h"

#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "logger.h"

#if !defined(DEFAULT_INPUT_CURRENT)
#define DEFAULT_INPUT_CURRENT			31818 /* milliampere */
#endif
#if !defined(DEFAULT_INPUT_VOLTAGE)
#define DEFAULT_INPUT_VOLTAGE			220 /* V */
#endif
#if !defined(DEFAULT_INPUT_FREQUENCY)
#define DEFAULT_INPUT_FREQUENCY			60 /* Hz */
#endif
#if !defined(DEFAULT_OUTPUT_CURRENT_MIN)
#define DEFAULT_OUTPUT_CURRENT_MIN		DEFAULT_INPUT_CURRENT
#endif
#if !defined(DEFAULT_OUTPUT_CURRENT_MAX)
#define DEFAULT_OUTPUT_CURRENT_MAX		DEFAULT_OUTPUT_CURRENT_MIN
#endif

struct connector_entry {
	struct list link; /* to charger.connectors.list */
	struct connector *connector;
	int id;
};

static bool validate_charger_param(const struct charger_param *param)
{
	if (!param) {
		return false;
	}

	if (param->max_input_current_mA == 0 ||
			param->input_voltage == 0 ||
			param->input_frequency == 0) {
		return false;
	}

	return true;
}

static int delete_connector(struct charger *self, struct connector *connector)
{
	struct list *head = &self->connectors.list;
	struct list *p;
	struct list *t;

	list_for_each_safe(p, t, head) {
		struct connector_entry *entry =
			list_entry(p, struct connector_entry, link);

		if (entry->connector == connector) {
			list_del(p, head);
			free(entry);
			return 0;
		}
	}

	return -ENOENT;
}

static void delete_connectors(struct charger *self)
{
	struct list *head = &self->connectors.list;
	struct list *p;
	struct list *t;

	list_for_each_safe(p, t, head) {
		struct connector_entry *entry =
			list_entry(p, struct connector_entry, link);

		list_del(p, head);
		free(entry);
	}
}

int charger_process(struct charger *self)
{
	struct list *p;

	if (self->extension.pre_process) {
		(*self->extension.pre_process)(self);
	}

	list_for_each(p, &self->connectors.list) {
		struct connector_entry *entry =
			list_entry(p, struct connector_entry, link);
		struct connector *connector = entry->connector;

		connector_process(connector);
	}

	if (self->extension.post_process) {
		(*self->extension.post_process)(self);
	}

	return 0;
}

int charger_get_connector_id(const struct charger *self,
		const struct connector *connector, int *id)
{
	struct list *p;

	if (!connector || !id) {
		return -EINVAL;
	}

	list_for_each(p, &self->connectors.list) {
		struct connector_entry *entry =
			list_entry(p, struct connector_entry, link);

		if (entry->connector == connector) {
			*id = entry->id;
			return 0;
		}
	}

	return -ENOENT;
}

void charger_iterate_connectors(struct charger *self,
		charger_iterate_cb_t cb, void *cb_ctx)
{
	struct list *p;

	list_for_each(p, &self->connectors.list) {
		struct connector_entry *entry =
			list_entry(p, struct connector_entry, link);

		cb(entry->connector, cb_ctx);
	}
}

void charger_default_param(struct charger_param *param)
{
	if (param) {
		*param = (struct charger_param) {
			.max_input_current_mA = DEFAULT_INPUT_CURRENT,
			.input_voltage = DEFAULT_INPUT_VOLTAGE,
			.input_frequency = DEFAULT_INPUT_FREQUENCY,
			.max_output_current_mA = DEFAULT_OUTPUT_CURRENT_MAX,
			.min_output_current_mA = DEFAULT_OUTPUT_CURRENT_MIN,
		};
	}
}

struct connector *charger_get_connector(const struct charger *self,
		const char *connector_name_str)
{
	struct list *p;

	list_for_each(p, &self->connectors.list) {
		struct connector_entry *entry =
			list_entry(p, struct connector_entry, link);
		struct connector *c = entry->connector;

		if (strcmp(connector_name(c), connector_name_str) == 0) {
			return c;
		}
	}

	return NULL;
}

struct connector *charger_get_connector_available(const struct charger *self)
{
	struct list *p;

	list_for_each(p, &self->connectors.list) {
		struct connector_entry *entry =
			list_entry(p, struct connector_entry, link);
		struct connector *c = entry->connector;

		if (connector_is_enabled(c) && !connector_is_reserved(c) &&
				connector_state(c) == A) {
			return c;
		}
	}

	return NULL;
}

struct connector *charger_get_connector_by_id(const struct charger *self,
		const int id)
{
	struct list *p;

	list_for_each(p, &self->connectors.list) {
		struct connector_entry *entry =
			list_entry(p, struct connector_entry, link);

		if (entry->id == id) {
			return entry->connector;
		}
	}

	return NULL;
}

int charger_count_connectors(const struct charger *self)
{
	struct list *p;
	int count = 0;

	list_for_each(p, &self->connectors.list) {
		count++;
	}

	return count;
}

int charger_register_event_cb(struct charger *self,
		charger_event_cb_t cb, void *cb_ctx)
{
	self->event_cb = cb;
	self->event_cb_ctx = cb_ctx;
	return 0;
}

int charger_attach_connector(struct charger *self, struct connector *connector)
{
	struct connector_entry *entry = malloc(sizeof(*entry));

	if (entry == NULL) {
		return -ENOMEM;
	}

	self->connectors.id_counter++;

	entry->connector = connector;
	entry->id = self->connectors.id_counter;
	list_add_tail(&entry->link, &self->connectors.list);

	connector->id = entry->id;

	return 0;
}

int charger_detach_connector(struct charger *self, struct connector *connector)
{
	return delete_connector(self, connector);
}

int charger_init(struct charger *self, const struct charger_param *param,
		const struct charger_extension *extension)
{
	if (!validate_charger_param(param)) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	memcpy(&self->param, param, sizeof(*param));
	list_init(&self->connectors.list);

	if (extension) {
		memcpy(&self->extension, extension, sizeof(*extension));
		(*extension->init)(self);
	}

	return 0;
}

void charger_deinit(struct charger *self)
{
	if (self->extension.deinit) {
		(*self->extension.deinit)(self);
	}

	delete_connectors(self);
}
