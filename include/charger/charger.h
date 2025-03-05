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

struct charger;
struct connector;

typedef uint32_t charger_event_t;
typedef void (*charger_event_cb_t)(struct charger *charger, struct connector *c,
		charger_event_t event, void *ctx);
typedef void (*charger_iterate_cb_t)(struct connector *c, void *ctx);

struct charger_param {
	uint32_t max_input_current_mA; /* maximum input current in mA */
	int16_t input_voltage; /* input voltage in V */
	int16_t input_frequency; /* input frequency in Hz */

	uint32_t max_output_current_mA; /* maximum output current in mA */
	uint32_t min_output_current_mA; /* minimum output current in mA */
};

struct charger_extension {
	int (*init)(struct charger *self);
	void (*deinit)(struct charger *self);
	int (*pre_process)(struct charger *self);
	int (*post_process)(struct charger *self);
};

/**
 * @brief Initializes the charger with the given parameters and extension.
 *
 * @param[in] self Pointer to the charger instance to initialize.
 * @param[in] param Pointer to the charger parameters.
 * @param[in] extension Pointer to the charger extension.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int charger_init(struct charger *self, const struct charger_param *param,
		const struct charger_extension *extension);

/**
 * @brief Deinitialize the charger.
 *
 * @param[in] self Pointer to the charger instance to initialize.
 */
void charger_deinit(struct charger *self);

/**
 * @brief Attach a connector to the charger.
 *
 * @param[in] self Pointer to the charger instance to initialize.
 * @param[in] connector Pointer to the connector structure.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int charger_attach_connector(struct charger *self, struct connector *connector);

/**
 * @brief Detach a connector from the charger.
 *
 * @param[in] self Pointer to the charger instance to initialize.
 * @param[in] connector Pointer to the connector structure.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int charger_detach_connector(struct charger *self, struct connector *connector);

/**
 * @brief Register an event callback for the charger.
 *
 * This function registers a callback function that will be called when an event
 * occurs on the charger.
 *
 * @param[in] self Pointer to the charger instance to initialize.
 * @param[in] cb The callback function to register.
 * @param[in] cb_ctx User-defined context to pass to the callback function.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int charger_register_event_cb(struct charger *self,
		charger_event_cb_t cb, void *cb_ctx);

/**
 * @brief Get the count of connectors attached to the charger.
 *
 * @param[in] self Pointer to the charger instance to initialize.
 *
 * @return The number of connectors attached to the charger.
 */
int charger_count_connectors(const struct charger *self);

/**
 * @brief Process the charger state.
 *
 * @param[in] self Pointer to the charger instance to initialize.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int charger_process(struct charger *self);

/**
 * @brief Get a connector by its name.
 *
 * @param[in] self Pointer to the charger instance to initialize.
 * @param[in] connector_name_str The name of the connector.
 *
 * @return Pointer to the connector structure, or NULL if not found.
 */
struct connector *charger_get_connector(const struct charger *self,
		const char *connector_name_str);

/**
 * @brief Get an available connector.
 *
 * @param[in] self Pointer to the charger instance to initialize.
 *
 * @return Pointer to an available connector structure, or NULL if none are
 *         available.
 */
struct connector *charger_get_connector_available(const struct charger *self);

/**
 * @brief Get a connector by its ID.
 *
 * @param[in] self Pointer to the charger instance to initialize.
 * @param[in] id The ID of the connector.
 *
 * @return Pointer to the connector structure, or NULL if not found.
 */
struct connector *charger_get_connector_by_id(const struct charger *self,
		const int id);

/**
 * Iterates over all connectors of the charger and applies the given callback
 * function.
 *
 * @param[in] self Pointer to the charger instance.
 * @param[in] cb Callback function to be applied to each connector.
 * @param[in] ctx User-defined context to be passed to the callback function.
 */
void charger_iterate_connectors(struct charger *self,
		charger_iterate_cb_t cb, void *cb_ctx);

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
 * @brief Get the connector ID for a given charger and connector.
 *
 * @param[in] self Pointer to the charger structure.
 * @param[in] connector Pointer to the connector structure.
 * @param[out] id Pointer to an integer where the connector ID will be stored.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int charger_get_connector_id(const struct charger *self,
		const struct connector *connector, int *id);

#if defined(__cplusplus)
}
#endif

#endif /* CHARGER_H */
