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

#ifndef ADC122S051_H
#define ADC122S051_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include "libmcu/spi.h"

/**
 * @brief Callback function type for processing ADC samples.
 *
 * This typedef defines a callback function type that can be used to process
 * raw ADC samples. The callback function takes a context pointer and a raw
 * ADC value as input, and returns a processed value.
 *
 * @param[in,out] ctx Pointer to the user-defined context.
 * @param[in] adc_raw The raw ADC value to be processed.
 *
 * @return uint16_t The processed ADC value.
 */
typedef uint16_t (*adc122s051_callback_t)(void *ctx, const uint16_t adc_raw);

struct adc122s051_callback {
	adc122s051_callback_t on_sample;
	void *on_sample_ctx;
};

struct adc122s051;

/**
 * @brief Creates an instance of the adc122s051 structure.
 *
 * This function initializes and allocates memory for an adc122s051 structure
 * with the specified SPI device, buffer, and buffer size.
 *
 * @param[in] spi Pointer to the SPI device structure.
 * @param[in] buf Pointer to the buffer to be used for ADC samples.
 * @param[in] bufsize Size of the buffer to be allocated for ADC samples.
 *
 * @return struct adc122s051* Pointer to the created adc122s051 structure.
 */
struct adc122s051 *adc122s051_create(struct spi_device *spi,
		uint16_t *buf, const uint16_t bufsize);

/**
 * @brief Destroys an instance of the adc122s051 structure.
 *
 * This function deallocates the memory and cleans up resources associated
 * with the specified adc122s051 structure.
 *
 * @param[in,out] self Pointer to the adc122s051 structure to be destroyed.
 */
void adc122s051_destroy(struct adc122s051 *self);

/**
 * @brief Measures ADC samples and stores them in the provided array.
 *
 * This function measures a specified number of samples from the ADC and
 * stores the sampled millivolt values in the provided array. It also allows
 * a user-defined callback structure to process each sample.
 *
 * @param[in,out] self Pointer to the adc122s051 structure.
 * @param[out] adc_samples Array to store the sampled millivolt values.
 * @param[in] nr_samples Number of samples to measure from the ADC.
 * @param[in] cb Pointer to the adc122s051_callback structure containing
 *               callback functions.
 *
 * @return int Status code (0 for success, non-zero for error).
 */
int adc122s051_measure(struct adc122s051 *self,
		uint16_t adc_samples[], const uint16_t nr_samples,
		struct adc122s051_callback *cb);

/**
 * @brief Converts raw ADC value to millivolts.
 *
 * This function converts a raw ADC value to its corresponding millivolt
 * value based on the ADC's reference voltage and resolution.
 *
 * @param[in] adc_raw The raw ADC value to be converted.
 *
 * @return uint16_t The converted millivolt value.
 */
uint16_t adc122s051_convert_adc_to_millivolt(const uint16_t adc_raw);

#if defined(__cplusplus)
}
#endif

#endif /* ADC122S051_H */
