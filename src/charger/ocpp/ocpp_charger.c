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

#include "charger/ocpp.h"
#include "charger/ocpp_connector.h"
#include "ocpp_charger_internal.h"

#include <stdlib.h>
#include <string.h>

#include "libmcu/msgq.h"
#include "libmcu/compiler.h"

#define MAX_MESSAGES		4

struct iterator_ctx {
	struct connector *connector;
	const uint8_t *mid;
	const size_t mid_len;
	const ocpp_transaction_id_t tid;
};

static void on_each_connector_tid(struct connector *c, void *ctx)
{
	struct iterator_ctx *p = (struct iterator_ctx *)ctx;
	if (ocpp_connector_has_transaction(c, p->tid)) {
		p->connector = c;
	}
}

struct connector *ocpp_charger_get_connector_by_tid(struct charger *charger,
		const ocpp_transaction_id_t transaction_id)
{
	struct iterator_ctx ctx = {
		.connector = NULL,
		.tid = transaction_id,
	};

	charger_iterate_connectors(charger, on_each_connector_tid, &ctx);

	return ctx.connector;
}

void ocpp_charger_set_availability(struct charger *charger, bool available)
{
	struct ocpp_charger *ocpp_charger = (struct ocpp_charger *)charger;
	ocpp_charger->checkpoint.unavailable = !available;
}

bool ocpp_charger_get_availability(const struct charger *charger)
{
	const struct ocpp_charger *ocpp_charger =
		(const struct ocpp_charger *)charger;
	return !ocpp_charger->checkpoint.unavailable;
}

ocpp_charger_reboot_t
ocpp_charger_get_pending_reboot_type(const struct charger *charger)
{
	const struct ocpp_charger *ocpp_charger =
		(const struct ocpp_charger *)charger;
	return ocpp_charger->reboot_required;
}

int ocpp_charger_request_reboot(struct charger *charger,
		ocpp_charger_reboot_t type)
{
	struct ocpp_charger *ocpp_charger = (struct ocpp_charger *)charger;
	ocpp_charger->reboot_required = type;
	return 0;
}

int ocpp_charger_mq_send(struct charger *charger,
		ocpp_charger_msg_t type, void *value)
{
	struct ocpp_charger *ocpp_charger = (struct ocpp_charger *)charger;
	struct ocpp_charger_msg msg = {
		.type = type,
		.value = value,
	};
	return msgq_push(ocpp_charger->msgq, &msg, sizeof(msg));
}

int ocpp_charger_mq_recv(struct charger *charger, struct ocpp_charger_msg *msg)
{
	struct ocpp_charger *ocpp_charger = (struct ocpp_charger *)charger;
	return msgq_pop(ocpp_charger->msgq, msg, sizeof(*msg));
}

struct ocpp_checkpoint *ocpp_charger_get_checkpoint(struct charger *charger)
{
	struct ocpp_charger *ocpp_charger = (struct ocpp_charger *)charger;
	return &ocpp_charger->checkpoint;
}

int ocpp_charger_set_checkpoint(struct charger *charger,
		const struct ocpp_checkpoint *checkpoint)
{
	struct ocpp_charger *ocpp_charger = (struct ocpp_charger *)charger;
	ocpp_charger->checkpoint = *checkpoint;
	return 0;
}

bool ocpp_charger_is_checkpoint_equal(const struct charger *charger,
		const struct ocpp_checkpoint *checkpoint)
{
	const struct ocpp_charger *ocpp_charger =
		(const struct ocpp_charger *)charger;
	return memcmp(checkpoint, &ocpp_charger->checkpoint,
			sizeof(*checkpoint)) == 0;
}

struct charger *ocpp_charger_create(void *ctx)
{
	unused(ctx);
	struct ocpp_charger *ocpp_charger = calloc(1, sizeof(*ocpp_charger));

	if (!ocpp_charger) {
		return NULL;
	}

	if ((ocpp_charger->msgq = msgq_create(msgq_calc_size(MAX_MESSAGES,
			sizeof(struct ocpp_charger_msg)))) == NULL) {
		free(ocpp_charger);
		return NULL;
	}

	return &ocpp_charger->base;
}

void ocpp_charger_destroy(struct charger *charger)
{
	struct ocpp_charger *ocpp_charger = (struct ocpp_charger *)charger;
	msgq_destroy(ocpp_charger->msgq);
	free(ocpp_charger);
}
