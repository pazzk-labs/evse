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

#ifndef SAFETY_H
#define SAFETY_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

typedef enum {
	SAFETY_STATUS_UNKNOWN,
	SAFETY_STATUS_OK,
	SAFETY_STATUS_ABNORMAL_FREQUENCY,
	SAFETY_STATUS_SAMPLING_ERROR, /**< Not enough samples to determine the
					frequency. */
	SAFETY_STATUS_STALE, /**< The frequency is not updated for a long time.
				This will happen when the power is not normal,
				keeping the line high. */
	SAFETY_STATUS_EMERGENCY_STOP, /**< This status is checked only in
				the SAFETY_TYPE_OUTPUT_POWER type. */
} safety_status_t;

typedef enum {
	SAFETY_TYPE_ALL,
	SAFETY_TYPE_INPUT_POWER,
	SAFETY_TYPE_OUTPUT_POWER,
} safety_t;

struct lm_gpio;

/**
 * @brief Initializes the safety module.
 *
 * This function initializes the safety module with the specified input and
 * output power GPIOs. It sets up the necessary configurations for power
 * monitoring.
 *
 * @param[in] input_power Pointer to the GPIO structure for input power.
 * @param[in] output_power Pointer to the GPIO structure for output power.
 *
 * @return int Status code (0 for success, non-zero for error).
 */
int safety_init(struct lm_gpio *input_power, struct lm_gpio *output_power);

/**
 * @brief Deinitializes the safety module.
 *
 * This function deinitializes the safety module, cleaning up any resources
 * that were allocated during initialization.
 */
void safety_deinit(void);

/**
 * @brief Enables the safety module.
 *
 * This function enables the safety module, allowing it to start monitoring
 * input and output power.
 *
 * @return int Status code (0 for success, non-zero for error).
 */
int safety_enable(void);

/**
 * @brief Disables the safety module.
 *
 * This function disables the safety module, stopping it from monitoring
 * input and output power.
 *
 * @return int Status code (0 for success, non-zero for error).
 */
int safety_disable(void);

/**
 * @brief Retrieves the current safety status.
 *
 * This function retrieves the current safety status for the specified type
 * and compares it with the expected frequency.
 *
 * @param[in] type The type of safety status to retrieve.
 * @param[in] expected_freq The expected frequency to compare against.
 *
 * @return safety_status_t The current safety status.
 */
safety_status_t safety_status(safety_t type, const uint8_t expected_freq);

/**
 * @brief Retrieves the frequency of the power signals.
 *
 * This function retrieves the frequency of the power signals for the
 * specified type.
 *
 * @param[in] type The type of power signal to retrieve the frequency for.
 *
 * @return uint8_t The frequency of the power signals.
 */
uint8_t safety_get_frequency(safety_t type);

#if defined(__cplusplus)
}
#endif

#endif /* SAFETY_H */
