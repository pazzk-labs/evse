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

#ifndef CHARGER_CONNECTOR_H
#define CHARGER_CONNECTOR_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include "iec61851.h"
#include "metering.h"

typedef enum {
	CONNECTOR_ERROR_NONE,
	CONNECTOR_ERROR_EV_SIDE,
	CONNECTOR_ERROR_EVSE_SIDE,
	CONNECTOR_ERROR_EMERGENCY_STOP,
} connector_error_t;

struct connector_param {
	uint32_t max_output_current_mA; /* maximum output current in mA */
	uint32_t min_output_current_mA; /* minimum output current in mA */

	struct iec61851 *iec61851;
	struct metering *metering;

	const char *name;
	int priority;
};

struct charger;
struct connector;

/**
 * @brief Reports an error for a specific charger connector.
 *
 * This function is used to report an error associated with a specific
 * charger connector. It takes a pointer to the charger and a pointer
 * to the charger connector as parameters.
 *
 * @param[in] connector Pointer to the charger connector structure.
 *
 * @return connector_error_t Error code indicating the type of error.
 */
connector_error_t connector_error(const struct connector *connector);

/**
 * @brief Set the priority of a charger connector.
 *
 * This function sets the priority of the specified charger connector.
 *
 * @param[in] connector_name Name of the connector.
 * @param[in] priority The priority to set.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int connector_set_priority(struct connector *connector, const int priority);

/**
 * @brief Retrieves the status of a specific charger connector.
 *
 * This function returns the status of a charger connector given the
 * charger structure and the name of the connector.
 *
 * @param[in] connector_name Name of the charger connector.
 *
 * @return const char* Status of the specified charger connector.
 */
const char *connector_status(const struct connector *connector);

/**
 * @brief Validate connector parameters.
 *
 * This function validates the parameters of a connector against the specified
 * maximum input current.
 *
 * @param[in] param A pointer to the connector parameters to be validated.
 * @param[in] max_input_current The maximum allowable input current for the
 *            connector.
 *
 * @return true if the parameters are valid, false otherwise.
 */
bool connector_validate_param(const struct connector_param *param,
		const uint32_t max_input_current);

#if defined(__cplusplus)
}
#endif

#endif /* CHARGER_CONNECTOR_H */
