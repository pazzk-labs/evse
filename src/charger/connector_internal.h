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

#ifndef CHARGER_CONNECTOR_INTERNAL_H
#define CHARGER_CONNECTOR_INTERNAL_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "charger/connector.h"

/**
 * @brief Get the target duty cycle of the connector.
 *
 * @param[in] self Pointer to the connector structure.
 *
 * @return The target duty cycle as a uint8_t.
 */
uint8_t connector_get_target_duty(const struct connector *self);

/**
 * @brief Get the actual duty cycle of the connector.
 *
 * @param[in] self Pointer to the connector structure.
 *
 * @return The actual duty cycle as a uint8_t.
 */
uint8_t connector_get_actual_duty(const struct connector *self);

/**
 * @brief Start the duty cycle of the connector.
 *
 * Sets an appropriate duty cycle according to the configuration and outputs
 * PWM.
 *
 * @param[in] self Pointer to the connector structure.
 */
void connector_start_duty(struct connector *self);

/**
 * @brief Stop the duty cycle of the connector.
 *
 * Sets the duty cycle to 100% and outputs PWM.
 *
 * @param[in] self Pointer to the connector structure.
 */
void connector_stop_duty(struct connector *self);

/**
 * @brief Transition the connector to a fault state.
 *
 * This function sets the connector to a fault state, indicating an error
 * condition.
 *
 * @param[in] self Pointer to the connector structure.
 */
void connector_go_fault(struct connector *self);

/**
 * @brief Enable the power supply for the connector.
 *
 * @param[in] self Pointer to the connector structure.
 */
void connector_enable_power_supply(struct connector *self);

/**
 * @brief Disable the power supply for the connector.
 *
 * @param[in] self Pointer to the connector structure.
 */
void connector_disable_power_supply(struct connector *self);

/**
 * @brief Check if the connector is supplying power.
 *
 * @param[in] self Pointer to the connector structure.
 *
 * @return true if the connector is supplying power, false otherwise.
 */
bool connector_is_supplying_power(const struct connector *self);

/**
 * @brief Check if the connector is in a specific state.
 *
 * @param[in] self Pointer to the connector structure.
 * @param[in] state The state to check against.
 *
 * @return true if the connector is in the specified state, false otherwise.
 */
bool connector_is_state_x(const struct connector *self,
		connector_state_t state);

/**
 * @brief Check if the connector is in a specific state.
 *
 * @param[in] self Pointer to the connector structure.
 * @param[in] state The state to check against.
 *
 * @return true if the connector is in the specified state, false otherwise.
 */
bool connector_is_state_x2(const struct connector *self,
		connector_state_t state);

/**
 * @brief Checks if the given state indicates an occupied connector.
 *
 * This function determines if the provided connector state indicates
 * that the connector is currently occupied.
 *
 * @param state The state of the connector to check.
 *
 * @return true if the state indicates the connector is occupied, false
 *         otherwise.
 */
bool connector_is_occupied_state(const connector_state_t state);

/**
 * @brief Check if the connector is in an EVSE error state.
 *
 * @param[in] self Pointer to the connector structure.
 * @param[in] state The state to check against.
 *
 * @return true if the connector is in an EVSE error state, false otherwise.
 */
bool connector_is_evse_error(struct connector *self, connector_state_t state);

/**
 * @brief Check if the connector is in an emergency stop state.
 *
 * @param[in] self Pointer to the connector structure.
 *
 * @return true if the connector is in an emergency stop state, false otherwise.
 */
bool connector_is_emergency_stop(const struct connector *self);

/**
 * @brief Check if the input power is OK for the connector.
 *
 * @param[in] self Pointer to the connector structure.
 *
 * @return true if the input power is OK, false otherwise.
 */
bool connector_is_input_power_ok(struct connector *self);

/**
 * @brief Check if the output power is OK for the connector.
 *
 * @param[in] self Pointer to the connector structure.
 *
 * @return true if the output power is OK, false otherwise.
 */
bool connector_is_output_power_ok(struct connector *self);

/**
 * @brief Check if there is an EV response timeout.
 *
 * This function checks if the elapsed time since the last EV response exceeds
 * the timeout threshold.
 *
 * @param[in] self Pointer to the connector structure.
 * @param[in] elapsed_sec The elapsed time in seconds.
 *
 * @return true if there is an EV response timeout, false otherwise.
 */
bool connector_is_ev_response_timeout(const struct connector *self,
		uint32_t elapsed_sec);

/**
 * @brief Set the connector as reserved.
 *
 * @param[in] self Pointer to the connector structure.
 *
 * @return true on success, false otherwise.
 */
bool connector_set_reserved(struct connector *self);

/**
 * @brief Clear the reserved status of the connector.
 *
 * @param[in] self Pointer to the connector structure.
 *
 * @return true on success, false otherwise.
 */
bool connector_clear_reserved(struct connector *self);

/**
 * @brief Validate the connector parameters.
 *
 * @param[in] param Pointer to the connector parameter structure.
 *
 * @return true if the parameters are valid, false otherwise.
 */
bool connector_validate_param(const struct connector_param *param);

/**
 * @brief Convert the connector state to a string representation.
 *
 * @param[in] state The connector state.
 *
 * @return The string representation of the state.
 */
const char *connector_stringify_state(const connector_state_t state);

/**
 * @brief Get the error state of the connector.
 *
 * @param[in] self Pointer to the connector structure.
 *
 * @return The error state of the connector.
 */
connector_error_t connector_error(const struct connector *self);

/**
 * @brief Updates the metrics for the given connector state.
 *
 * This function updates various metrics based on the current state
 * of the connector. It should be called whenever the state of the
 * connector changes to ensure metrics are accurately maintained.
 *
 * @param state The current state of the connector.
 */
void connector_update_metrics(const connector_state_t state);

#if defined(__cplusplus)
}
#endif

#endif /* CHARGER_CONNECTOR_INTERNAL_H */
