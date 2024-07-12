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

#ifndef METERING_H
#define METERING_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include "libmcu/uart.h"

typedef enum {
	METERING_HLW811X,
} metering_t;

struct metering;

struct metering_param {
	uint64_t wh; /**< energy in watt-hour */
	uint64_t varh; /**< reactive energy in volt-ampere-reactive-hour */
};

struct metering_io {
	struct uart *uart;
};

struct metering_api {
	void (*destroy)(struct metering *self);
	int (*enable)(struct metering *self);
	int (*disable)(struct metering *self);
	int (*step)(struct metering *self);
	int (*get_voltage)(struct metering *self, int32_t *millivolt);
	int (*get_current)(struct metering *self, int32_t *milliamp);
	int (*get_power_factor)(struct metering *self, int32_t *centi);
	int (*get_frequency)(struct metering *self, int32_t *centihertz);
	int (*get_phase)(struct metering *self, int32_t *centidegree, int hz);
	int (*get_energy)(struct metering *self, uint64_t *wh, uint64_t *varh);
	int (*get_power)(struct metering *self, int32_t *watt, int32_t *var);
};

/**
 * @brief Creates a metering instance.
 *
 * This function initializes a metering instance based on the specified type,
 * parameters, and I/O configuration.
 *
 * @param[in] type The type of metering to be created.
 * @param[in] param A pointer to the metering parameters.
 * @param[in] io A pointer to the metering I/O configuration.
 *
 * @return A pointer to the created metering instance.
 */
struct metering *metering_create(const metering_t type,
		const struct metering_param *param, struct metering_io *io);

/**
 * @brief Destroys a metering instance.
 *
 * This function releases the resources associated with the given metering
 * instance.
 *
 * @param[in] self A pointer to the metering instance to be destroyed.
 */
void metering_destroy(struct metering *self);

/**
 * @brief Enables the metering instance.
 *
 * This function enables the metering process for the given metering instance.
 *
 * @param[in] self A pointer to the metering instance to be enabled.
 *
 * @return 0 on success, non-zero on failure.
 */
int metering_enable(struct metering *self);

/**
 * @brief Disables the metering instance.
 *
 * This function disables the metering process for the given metering instance.
 *
 * @param[in] self A pointer to the metering instance to be disabled.
 *
 * @return 0 on success, non-zero on failure.
 */
int metering_disable(struct metering *self);

/**
 * @brief Executes a metering step.
 *
 * This function performs a metering process using the `step` method
 * of the `metering_api` structure associated with the given `metering`
 * instance. This function is intended to be called periodically.
 *
 * @param[in] self A pointer to the `metering` instance.
 *
 * @return 0 on success, non-zero on failure.
 */
int metering_step(struct metering *self);

/**
 * @brief Get the voltage measurement.
 *
 * @param[in] self Pointer to the metering structure.
 * @param[out] millivolt Pointer to store the voltage in millivolts.
 *
 * @return 0 on success, non-zero on failure.
 */
int metering_get_voltage(struct metering *self, int32_t *millivolt);

/**
 * @brief Get the current measurement.
 *
 * @param[in] self Pointer to the metering structure.
 * @param[out] milliamp Pointer to store the current in milliamps.
 *
 * @return 0 on success, non-zero on failure.
 */
int metering_get_current(struct metering *self, int32_t *milliamp);

/**
 * @brief Get the power factor measurement.
 *
 * @param[in] self Pointer to the metering structure.
 * @param[out] centi Pointer to store the power factor in centi-units.
 *
 * @return 0 on success, non-zero on failure.
 */
int metering_get_power_factor(struct metering *self, int32_t *centi);

/**
 * @brief Get the frequency measurement.
 *
 * @param[in] self Pointer to the metering structure.
 * @param[out] centihertz Pointer to store the frequency in centihertz.
 *
 * @return 0 on success, non-zero on failure.
 */
int metering_get_frequency(struct metering *self, int32_t *centihertz);

/**
 * @brief Get the phase measurement.
 *
 * @param[in] self Pointer to the metering structure.
 * @param[out] centidegree Pointer to store the phase in centidegrees.
 * @param[in] hz Frequency in hertz.
 *
 * @return 0 on success, non-zero on failure.
 */
int metering_get_phase(struct metering *self, int32_t *centidegree, int hz);

/**
 * @brief Get the energy measurement.
 *
 * @param[in] self Pointer to the metering structure.
 * @param[out] wh Pointer to store the energy in watt-hours.
 * @param[out] varh Pointer to store the reactive energy in volt-ampere reactive
 *             hours.
 *
 * @return 0 on success, non-zero on failure.
 */
int metering_get_energy(struct metering *self, uint64_t *wh, uint64_t *varh);

/**
 * @brief Get the power measurement.
 *
 * @param[in] self Pointer to the metering structure.
 * @param[out] watt Pointer to store the power in watts.
 * @param[out] var Pointer to store the reactive power in volt-amperes reactive.
 *
 * @return 0 on success, non-zero on failure.
 */
int metering_get_power(struct metering *self, int32_t *watt, int32_t *var);

#if defined(__cplusplus)
}
#endif

#endif /* METERING_H */
