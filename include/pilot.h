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

#ifndef PILOT_H
#define PILOT_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

struct lm_pwm_channel;
struct adc122s051;

#if !defined(PILOT_NUMBER_OF_SAMPLES)
/* 500 samples per 1ms, which is a cycle of 1kHz, at 8MHz 500kSPS ADC. */
#define PILOT_NUMBER_OF_SAMPLES		500
#endif

typedef enum {
	PILOT_STATUS_A			= 12,
	PILOT_STATUS_B			= 9,
	PILOT_STATUS_C			= 6,
	PILOT_STATUS_D			= 3,
	PILOT_STATUS_E			= 0,
	PILOT_STATUS_F			= -12,
	PILOT_STATUS_UNKNOWN		= 0xff,
} pilot_status_t;

typedef enum {
	PILOT_ERROR_NONE,
	PILOT_ERROR_DUTY_MISMATCH,
	PILOT_ERROR_FLUCTUATING, /* voltage is not within the boundaries. */
	PILOT_ERROR_TOO_LONG_INTERVAL,
	PILOT_ERROR_INVALID_PARAMS,
	PILOT_ERROR_NOT_INITIALIZED,
	PILOT_ERROR_UNKNOWN,
} pilot_error_t;

struct pilot_boundary {
	uint16_t a;
	uint16_t b;
	uint16_t c;
	uint16_t d;
	uint16_t e;
};

struct pilot_boundaries {
	struct pilot_boundary upward;
	struct pilot_boundary downward;
};

struct pilot_params {
	uint16_t scan_interval_ms; /* ADC scan interval in unit of ms */
	uint16_t cutoff_voltage_mv; /* high/low voltage cutoff in unit of mV */
	uint16_t noise_tolerance_mv; /* noise tolerance in unit of mV */
	uint16_t max_transition_clocks; /* max transition time in unit of clocks */

	struct pilot_boundaries boundary;

	uint16_t sample_count; /* number of samples for ADC measurements */
};

typedef void (*pilot_status_cb_t)(void *ctx, pilot_status_t status);

struct pilot;

/**
 * @brief Creates a pilot instance with the specified parameters.
 *
 * This function initializes and allocates memory for a pilot structure
 * with the specified parameters, ADC and PWM channel handles, and buffer for
 * ADC samples.
 *
 * @param[in] params Pointer to the pilot_params structure containing the
 *                   configuration parameters for the pilot instance.
 * @param[in] adc Pointer to the adc122s051 structure.
 * @param[in] pwm Pointer to the PWM channel structure.
 * @param[in] buf Pointer to the buffer to be used for ADC samples.
 *
 * @return struct pilot* Pointer to the created pilot structure.
 */
struct pilot *pilot_create(const struct pilot_params *params,
        struct adc122s051 *adc, struct lm_pwm_channel *pwm, uint16_t *buf);

/**
 * @brief Deletes a pilot instance.
 *
 * This function deallocates the memory and cleans up resources associated
 * with the specified pilot structure.
 *
 * @param[in] pilot Pointer to the pilot structure to be deleted.
 */
void pilot_delete(struct pilot *pilot);

/**
 * @brief Set the callback for pilot status changes.
 *
 * This function sets a callback function that will be called whenever the
 * status of the pilot changes. The callback function will be provided with
 * the current status and the user-defined context.
 *
 * @param[in] pilot Pointer to the pilot structure.
 * @param[in] cb The callback function to be called on status changes.
 * @param[in] cb_ctx User-defined context to be passed to the callback function.
 */
void pilot_set_status_cb(struct pilot *pilot,
		pilot_status_cb_t cb, void *cb_ctx);

/**
 * @brief Enables the pilot instance.
 *
 * This function enables the pilot instance, allowing it to start its
 * operations. It typically involves starting the PWM signal and any
 * necessary ADC measurements.
 *
 * @param[in,out] pilot Pointer to the pilot structure.
 *
 * @return int Status code (0 for success, non-zero for error).
 */
int pilot_enable(struct pilot *pilot);

/**
 * @brief Disables the pilot instance.
 *
 * This function disables the pilot instance, stopping its operations. It
 * typically involves stopping the PWM signal and any ongoing ADC
 * measurements.
 *
 * @param[in,out] pilot Pointer to the pilot structure.
 *
 * @return int Status code (0 for success, non-zero for error).
 */
int pilot_disable(struct pilot *pilot);

/**
 * @brief Sets the duty cycle for the pilot PWM signal.
 *
 * This function sets the PWM duty cycle for the pilot signal to the
 * specified percentage.
 *
 * @param[in,out] pilot Pointer to the pilot structure.
 * @param[in] pct Percentage of the PWM duty cycle to be set.
 *
 * @return int Status code (0 for success, non-zero for error).
 */
int pilot_set_duty(struct pilot *pilot, const uint8_t pct);

/**
 * @brief Get the duty cycle set by the user.
 *
 * This function retrieves the duty cycle percentage that has been set by the
 * user, not the value read from the PWM signal.
 *
 * @param[in] pilot Pointer to the pilot structure.
 *
 * @return The duty cycle percentage set by the user.
 */
uint8_t pilot_get_duty_set(const struct pilot *pilot);

/**
 * @brief Retrieves the current duty cycle of the pilot PWM signal.
 *
 * This function retrieves the current duty cycle percentage of the pilot
 * PWM signal. The duty cycle represents the proportion of time the signal
 * is active within a given period.
 *
 * @param[in] pilot Pointer to the pilot structure.
 *
 * @return uint8_t The current duty cycle percentage of the pilot PWM signal.
 */
uint8_t pilot_duty(const struct pilot *pilot);

/**
 * @brief Retrieves the current status of the pilot instance.
 *
 * This function retrieves the current status of the pilot instance and
 * returns it as a pilot_status_t structure. The status includes various
 * operational parameters and states of the pilot.
 *
 * @param[in] pilot Pointer to the pilot structure.
 *
 * @return pilot_status_t The current status of the pilot instance.
 */
pilot_status_t pilot_status(const struct pilot *pilot);

/**
 * @brief Retrieves the current millivolt value from the pilot instance.
 *
 * This function retrieves the current millivolt value measured by the
 * pilot instance. It typically involves reading the latest ADC measurement
 * and converting it to millivolts. The low_voltage parameter indicates
 * whether to retrieve the low voltage measurement.
 *
 * @param[in] pilot Pointer to the pilot structure.
 * @param[in] low_voltage Boolean indicating whether to retrieve the low voltage
 *                                 measurement.
 *
 * @return uint16_t The current millivolt value measured by the pilot.
 */
uint16_t pilot_millivolt(const struct pilot *pilot, const bool low_voltage);

/**
 * @brief Checks if the pilot instance is operating correctly.
 *
 * This function checks the status of the pilot instance and returns true
 * if it is operating correctly, otherwise returns false.
 *
 * @param[in,out] pilot Pointer to the pilot structure.
 *
 * @return bool True if the pilot is operating correctly, false otherwise.
 */
bool pilot_ok(const struct pilot *pilot);

/**
 * @brief Retrieves the current error status of the pilot instance.
 *
 * This function retrieves the current error status of the pilot instance
 * and returns it as a pilot_error_t enumeration value. The error status
 * indicates any issues or faults detected in the pilot instance.
 *
 * @param[in] pilot Pointer to the pilot structure.
 *
 * @return pilot_error_t The current error status of the pilot instance.
 */
pilot_error_t pilot_error(const struct pilot *pilot);

/**
 * @brief Converts a pilot status to its string representation.
 *
 * This function converts the specified pilot status to its corresponding
 * string representation. This can be useful for logging or displaying
 * pilot status in a human-readable format.
 *
 * @param[in] status The pilot status to be converted to a string.
 *
 * @return const char* The string representation of the specified pilot status.
 */
const char *pilot_stringify_status(const pilot_status_t status);

/**
 * @brief Retrieves the default parameters for the pilot instance.
 *
 * This function initializes the provided pilot_params structure with the
 * default parameters for the pilot instance. These default parameters can
 * be used to configure the pilot instance with standard settings.
 *
 * @param[out] params Pointer to the pilot_params structure to be initialized
 *               with default values.
 */
void pilot_default_params(struct pilot_params *params);

#if defined(HOST_BUILD)
void pilot_set_status(struct pilot *pilot, const pilot_status_t status);
#endif

#if defined(__cplusplus)
}
#endif

#endif /* PILOT_H */
