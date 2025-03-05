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

#ifndef IEC61851_H
#define IEC61851_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

#include "pilot.h"
#include "relay.h"

#define IEC61851_EV_RESPONSE_TIMEOUT_SEC	6 /* IEC 61851-1, seq. 10.2 */

typedef enum {
	IEC61851_STATE_A,
	IEC61851_STATE_B,
	IEC61851_STATE_C,
	IEC61851_STATE_D,
	IEC61851_STATE_E,
	IEC61851_STATE_F,
} iec61851_state_t;

struct iec61851;

/**
 * @brief Create a new IEC 61851 instance.
 * 
 * @param[in] pilot Pointer to the pilot structure.
 * @param[in] relay Pointer to the relay structure.
 *
 * @return Pointer to the created IEC 61851 instance.
 */
struct iec61851 *iec61851_create(struct pilot *pilot, struct relay *relay);

/**
 * @brief Destroy an IEC 61851 instance.
 * 
 * @param[in] self Pointer to the IEC 61851 instance to be destroyed.
 */
void iec61851_destroy(struct iec61851 *self);

/**
 * @brief Get the current state of the IEC 61851 instance.
 * 
 * @param[in] self Pointer to the IEC 61851 instance.
 *
 * @return Current state of the IEC 61851 instance.
 */
iec61851_state_t iec61851_state(struct iec61851 *self);

/**
 * @brief Start the PWM signal with a specified duty cycle.
 * 
 * @param[in] self Pointer to the IEC 61851 instance.
 * @param[in] pct Duty cycle percentage for the PWM signal.
 */
void iec61851_start_pwm(struct iec61851 *self, const uint8_t pct);

/**
 * @brief Set the current limit for the IEC 61851 instance.
 * 
 * @param[in] self Pointer to the IEC 61851 instance.
 * @param[in] milliampere Current limit in milliamperes.
 */
void iec61851_set_current(struct iec61851 *self, const uint32_t milliampere);

/**
 * @brief Stop the PWM signal.
 * 
 * @param[in] self Pointer to the IEC 61851 instance.
 */
void iec61851_stop_pwm(struct iec61851 *self);

/**
 * @brief Set the state to 'F' for the IEC 61851 instance.
 * 
 * @param[in] self Pointer to the IEC 61851 instance.
 */
void iec61851_set_state_f(struct iec61851 *self);

/**
 * @brief Get the current PWM duty cycle.
 * 
 * @param[in] self Pointer to the IEC 61851 instance.
 *
 * @return Current PWM duty cycle percentage.
 */
uint8_t iec61851_get_pwm_duty(struct iec61851 *self);

/**
 * @brief Get the PWM duty cycle target for the IEC 61851 protocol.
 *
 * This function retrieves the target duty cycle for the Pulse Width Modulation
 * (PWM) signal used in the IEC 61851 protocol.
 *
 * @param[in] self Pointer to the iec61851 structure.
 *
 * @return uint8_t The target PWM duty cycle.
 */
uint8_t iec61851_get_pwm_duty_target(struct iec61851 *self);

/**
 * @brief Start the power supply.
 * 
 * @param[in] self Pointer to the IEC 61851 instance.
 */
void iec61851_start_power_supply(struct iec61851 *self);

/**
 * @brief Stop the power supply.
 * 
 * @param[in] self Pointer to the IEC 61851 instance.
 */
void iec61851_stop_power_supply(struct iec61851 *self);

/**
 * @brief Check if the power supply is active.
 * 
 * @param[in] self Pointer to the IEC 61851 instance.
 *
 * @return true if the power supply is active, false otherwise.
 */
bool iec61851_is_supplying_power(struct iec61851 *self);

/**
 * @brief Check if the instance is in charging state.
 * 
 * @param[in] self Pointer to the IEC 61851 instance.
 *
 * @return true if the instance is charging, false otherwise.
 */
bool iec61851_is_charging(struct iec61851 *self);

/**
 * @brief Check if a given state is a charging state.
 * 
 * @param[in] state State to be checked.
 *
 * @return true if the state is a charging state, false otherwise.
 */
bool iec61851_is_charging_state(const iec61851_state_t state);

/**
 * @brief Check if the instance is occupied.
 * 
 * @param[in] self Pointer to the IEC 61851 instance.
 *
 * @return true if the instance is occupied, false otherwise.
 */
bool iec61851_is_occupied(struct iec61851 *self);

/**
 * @brief Check if a given state is an occupied state.
 * 
 * @param[in] state State to be checked.
 *
 * @return true if the state is an occupied state, false otherwise.
 */
bool iec61851_is_occupied_state(const iec61851_state_t state);

/**
 * @brief Check if the instance is in an error state.
 * 
 * @param[in] self Pointer to the IEC 61851 instance.
 *
 * @return true if the instance is in an error state, false otherwise.
 */
bool iec61851_is_error(struct iec61851 *self);

/**
 * @brief Check if a given state is an error state.
 * 
 * @param[in] state State to be checked.
 *
 * @return true if the state is an error state, false otherwise.
 */
bool iec61851_is_error_state(const iec61851_state_t state);

/**
 * @brief Convert the state to a string representation.
 *
 * This function converts the given state to its string representation.
 *
 * @param[in] state The state to convert.
 *
 * @return The string representation of the state.
 */
const char *iec61851_stringify_state(const iec61851_state_t state);

/**
 * @brief Converts duty cycle percentage to current in milliamperes.
 *
 * This function takes a duty cycle percentage value and converts it to the 
 * equivalent current in milliamperes.
 *
 * @param[in] duty The duty cycle percentage value.
 *
 * @return The equivalent current in milliamperes.
 */
uint32_t iec61851_duty_to_milliampere(const uint8_t duty);

/**
 * @brief Converts current in milliamperes to duty cycle percentage.
 *
 * This function takes a current value in milliamperes and converts it to the 
 * equivalent duty cycle percentage.
 *
 * @param[in] milliampere The current value in milliamperes.
 *
 * @return The equivalent duty cycle percentage.
 */
uint8_t iec61851_milliampere_to_duty(const uint32_t milliampere);

#if defined(__cplusplus)
}
#endif

#endif /* IEC61851_H */
