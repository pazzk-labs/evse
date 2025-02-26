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

#ifndef CHARGER_H
#define CHARGER_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "charger/event.h"

#define CHARGER_EVENT_STRING_MAXLEN		128

struct charger;
struct connector;
struct connector_param;

typedef void (*charger_event_cb_t)(struct charger *charger,
		struct connector *connector,
		const charger_event_t event, void *ctx);

struct charger_param {
	uint32_t max_input_current_mA; /* maximum input current in mA */
	int16_t input_voltage; /* input voltage in V */
	int16_t input_frequency; /* input frequency in Hz */

	uint32_t max_output_current_mA; /* maximum output current in mA */
	uint32_t min_output_current_mA; /* minimum output current in mA */
};

/**
 * @brief Initialize the charger with the given parameters.
 *
 * @param[in] charger Pointer to the charger structure to be initialized.
 * @param[in] param Pointer to the charger parameters structure.
 *
 * @return int Returns 0 on success, or a negative error code on failure.
 */
int charger_init(struct charger *charger, const struct charger_param *param);

/**
 * @brief Deinitializes the charger.
 *
 * This function deinitializes the charger, releasing any resources
 * that were allocated during its initialization and operation.
 *
 * @param[in] charger Pointer to the charger structure.
 *
 * @return int 0 on success, or a negative error code on failure.
 */
int charger_deinit(struct charger *charger);

/**
 * @brief Register an event callback for the charger.
 *
 * @param[in] charger Pointer to the charger structure.
 * @param[in] cb Event callback function.
 * @param[in] cb_ctx Context to be passed to the callback function.
 *
 * @return int Returns 0 on success, or a negative error code on failure.
 */
int charger_register_event_cb(struct charger *charger,
		charger_event_cb_t cb, void *cb_ctx);

/**
 * @brief Retrieves a charger connector by name.
 *
 * This function returns a pointer to the charger connector structure
 * that matches the given connector name.
 *
 * @param[in] charger Pointer to the charger structure.
 * @param[in] connector_name Name of the charger connector.
 *
 * @return struct connector* Pointer to the charger connector structure.
 */
struct connector *charger_get_connector(struct charger *charger,
		const char *connector_name);

/**
 * @brief Function to get an unused connector.
 *
 * This function retrieves an unused connector from the specified charger.
 *
 * @param[in] charger A pointer to the charger from which to get an unused
 *            connector.
 * @return A pointer to an unused connector, or NULL if no unused connector is
 *         available.
 */
struct connector *charger_get_connector_free(struct charger *charger);

/**
 * @brief Retrieves a charger connector by ID.
 *
 * This function returns a pointer to the charger connector structure
 * that matches the given connector ID.
 *
 * @param[in] charger Pointer to the charger structure.
 * @param[in] id ID of the charger connector.
 *
 * @return struct connector* Pointer to the charger connector structure.
 */
struct connector *charger_get_connector_by_id(struct charger *charger,
		const int id);

/**
 * @brief Creates a new charger connector.
 *
 * This function initializes and creates a new charger connector based on
 * the provided parameters. It takes a pointer to the charger structure
 * and a pointer to the charger connector parameters structure.
 *
 * @param[in] charger Pointer to the charger structure.
 * @param[in] param Pointer to the charger connector parameters structure.
 *
 * @return bool True if the connector was successfully created, false otherwise.
 */
bool charger_create_connector(struct charger *charger,
		const struct connector_param *param);

/**
 * @brief Perform a step in the charger process.
 *
 * This function performs a step in the charger process and determines
 * the next period in milliseconds.
 *
 * @param[in] charger Pointer to the charger structure.
 * @param[out] next_period_ms Pointer to the variable to store the next period
 *             in milliseconds.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int charger_step(struct charger *charger, uint32_t *next_period_ms);

/**
 * @brief Initializes the charger parameters to their default values.
 *
 * This function sets the provided charger_param structure to its default
 * values. It ensures that all fields are initialized properly before use.
 *
 * @param[in] param Pointer to the charger_param structure to be initialized.
 */
void charger_default_param(struct charger_param *param);

/**
 * Converts a charger event into its string representation
 *
 * @param[in]  event    The charger event to convert
 * @param[out] buf      Buffer to store the string representation
 * @param[in]  bufsize  Size of the provided buffer
 *
 * @return The number of characters written to the buffer (excluding null
 *         terminator)
 */
size_t charger_stringify_event(const charger_event_t event,
		char *buf, size_t bufsize);

#if defined(__cplusplus)
}
#endif

#endif /* CHARGER_H */
