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

#ifndef OCPP_CHECKPOINT_H
#define OCPP_CHECKPOINT_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include "libmcu/compiler.h"

#if !defined(OCPP_CONNECTOR_MAX)
#define OCPP_CONNECTOR_MAX		1
#endif

typedef uint32_t ocpp_transaction_id_t;

struct ocpp_connector_checkpoint {
	/* The following fields are used for power loss recovery. */
	ocpp_transaction_id_t transaction_id;
	bool unavailable; /* connector availability */
	uint8_t padding[3]; /* padding to ensure 8-byte alignment */
};
static_assert(sizeof(struct ocpp_connector_checkpoint) == 8,
		"Incorrect size of struct ocpp_connector_checkpoint");

struct ocpp_checkpoint {
	struct ocpp_connector_checkpoint connector[OCPP_CONNECTOR_MAX];

	bool unavailable; /* charger availability */
	bool fw_updated; /* firmware updated */
	uint8_t padding[6];
};
static_assert(sizeof(struct ocpp_checkpoint) == OCPP_CONNECTOR_MAX * 8 + 8,
		"Incorrect size of struct ocpp_checkpoint");

#if defined(__cplusplus)
}
#endif

#endif /* OCPP_CHECKPOINT_H */
