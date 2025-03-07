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

#ifndef OCPP_CONNECTOR_H
#define OCPP_CONNECTOR_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "charger/connector.h"
#include "charger/ocpp_checkpoint.h"

typedef enum {
	Booting,	/* 0 */
	Faulted,	/* 1 */
	Available,	/* 2 */
	Preparing,	/* 3 */
	Charging,	/* 4 */
	SuspendedEV,	/* 5 */
	SuspendedEVSE,	/* 6 */
	Finishing,	/* 7 */
	Reserved,	/* 8 */
	Unavailable,	/* 9 */
} ocpp_connector_state_t;

struct ocpp_connector;

/**
 * @brief Creates an OCPP connector.
 *
 * This function initializes and returns a new OCPP connector using the provided
 * parameters.
 *
 * @param[in] param A pointer to the connector parameters.
 *
 * @return A pointer to the newly created connector.
 */
struct connector *ocpp_connector_create(const struct connector_param *param);

/**
 * @brief Destroys an OCPP connector.
 *
 * This function cleans up and deallocates the resources associated with the
 * given OCPP connector.
 *
 * @param[in] c A pointer to the connector to be destroyed.
 */
void ocpp_connector_destroy(struct connector *c);

/**
 * @brief Links a checkpoint to the OCPP connector.
 *
 * This function associates a checkpoint with the given OCPP connector.
 *
 * @param[in] c A pointer to the connector.
 * @param[in] checkpoint A pointer to the checkpoint to be linked.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ocpp_connector_link_checkpoint(struct connector *c,
		struct ocpp_connector_checkpoint *checkpoint);

/**
 * @brief Checks if the OCPP connector has a specific transaction.
 *
 * This function checks if the given OCPP connector has a transaction with the
 * specified ID.
 *
 * @param[in] c A pointer to the connector.
 * @param[in] tid The transaction ID.
 *
 * @return A boolean indicating whether the transaction is present.
 */
bool ocpp_connector_has_transaction(struct connector *c,
		const ocpp_transaction_id_t tid);

#if defined(__cplusplus)
}
#endif

#endif /* OCPP_CONNECTOR_H */
