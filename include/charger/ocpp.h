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

#ifndef OCPP_CHARGER_H
#define OCPP_CHARGER_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "charger/charger.h"

#if !defined(OCPP_CHARGER_MAX_CONNECTORS)
#define OCPP_CHARGER_MAX_CONNECTORS		2
#endif

typedef uint32_t ocpp_charger_transaction_id_t;

struct ocpp_charger_unavailability {
	bool charger; /* true: unavailable, false: available */
	bool connector[OCPP_CHARGER_MAX_CONNECTORS];
};

struct ocpp_charger_param {
	ocpp_charger_transaction_id_t missing_transaction_id; /* power loss recovery */
	struct ocpp_charger_unavailability unavailability;
};

struct charger *ocpp_charger_create(const struct charger_param *param,
		const struct ocpp_charger_param *ocpp_param);
void ocpp_charger_destroy(struct charger *charger);

/**
 * @brief Backs up the transaction ID to a user-provided parameter.
 *
 * This function saves the transaction ID to a user-provided parameter to ensure
 * it can be recovered in case of a power failure or system reset. It should be
 * called when a transaction starts. The user is responsible for saving this
 * parameter to flash memory if needed.
 *
 * @param[in] connector Pointer to the connector structure containing the
 * transaction ID.
 */
void ocpp_charger_backup_tid(struct connector *connector);

/**
 * @brief Clears the backed-up transaction ID from the user-provided parameter.
 *
 * This function deletes the transaction ID from the user-provided parameter
 * after a transaction has successfully completed. It should be called to
 * prevent duplication of the transaction ID. The user is responsible for
 * clearing this parameter from flash memory if needed.
 *
 * @param[in] connector Pointer to the connector structure containing the
 * transaction ID.
 */
void ocpp_charger_clear_backup_tid(struct connector *connector);

/**
 * @brief Retrieves the OCPP charger parameters.
 *
 * This function returns a pointer to the OCPP charger parameters associated
 * with the specified charger. These parameters can be used to manage and
 * configure the charger, including handling transaction IDs for backup and
 * recovery purposes.
 *
 * @param[in] charger Pointer to the charger structure.
 *
 * @return Pointer to the OCPP charger parameters.
 */
const struct ocpp_charger_param *ocpp_charger_get_param(struct charger *charger);

#if defined(__cplusplus)
}
#endif

#endif /* OCPP_CHARGER_H */
