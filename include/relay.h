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

#ifndef RELAY_H
#define RELAY_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

struct relay;
struct lm_pwm_channel;

/**
 * @brief Creates a relay instance.
 *
 * This function initializes and allocates memory for a relay structure
 * with the specified PWM channel handle.
 *
 * @param[in] pwm_handle Pointer to the PWM channel structure.
 *
 * @return struct relay* Pointer to the created relay structure.
 */
struct relay *relay_create(struct lm_pwm_channel *pwm_handle);

/**
 * @brief Destroys a relay instance.
 *
 * This function deallocates the memory and cleans up resources associated
 * with the specified relay structure.
 *
 * @param[in,out] self Pointer to the relay structure to be destroyed.
 */
void relay_destroy(struct relay *self);

/**
 * @brief Enables the relay.
 *
 * This function enables the relay, allowing it to start its operations.
 * It typically involves starting the PWM signal or setting the appropriate
 * control signals to activate the relay.
 *
 * @param[in] self Pointer to the relay structure.
 *
 * @return int Status code (0 for success, non-zero for error).
 */
int relay_enable(struct relay *self);

/**
 * @brief Disables the relay.
 *
 * This function disables the relay, stopping its operations. It typically
 * involves stopping the PWM signal or resetting the control signals to
 * deactivate the relay.
 *
 * @param[in] self Pointer to the relay structure.
 *
 * @return int Status code (0 for success, non-zero for error).
 */
int relay_disable(struct relay *self);

/**
 * @brief Turns on the relay.
 *
 * This function activates the relay by setting the appropriate PWM signal.
 *
 * @param[in,out] self Pointer to the relay structure.
 */
void relay_turn_on(struct relay *self);

/**
 * @brief Turns off the relay.
 *
 * This function deactivates the relay by stopping the PWM signal.
 *
 * @param[in,out] self Pointer to the relay structure.
 */
void relay_turn_off(struct relay *self);

/**
 * @brief Turns on the relay with extended parameters.
 *
 * This function activates the relay with specified pickup and hold
 * percentages and a delay for the pickup phase.
 *
 * @param[in,out] self Pointer to the relay structure.
 * @param[in] pickup_pct Percentage of PWM duty cycle for the pickup phase.
 * @param[in] pickup_delay_ms Delay in milliseconds for the pickup phase.
 * @param[in] hold_pct Percentage of PWM duty cycle for the hold phase.
 */
void relay_turn_on_ext(struct relay *self,
        uint8_t pickup_pct, uint16_t pickup_delay_ms, uint8_t hold_pct);

#if defined(__cplusplus)
}
#endif

#endif /* RELAY_H */
