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

#ifndef PINMAP_H
#define PINMAP_H

#if defined(__cplusplus)
extern "C" {
#endif

struct gpio;
struct i2c;
struct spi;
struct pwm;
struct adc;
struct uart;

struct pinmap_periph {
	struct i2c *i2c0; /* io expander, accelerometer, codec, temperature */
	struct i2c_device *io_expander;
	struct i2c_device *acc;
	struct i2c_device *codec;
	struct i2c_device *temp;
	struct spi *spi2; /* qca7005, w5500 */
	struct spi_device *qca7005;
	struct spi_device *w5500;
	struct spi *spi3; /* adc */
	struct spi_device *cpadc;
	struct adc *adc1; /* board revision */
	struct pwm *pwm0; /* relay */
	struct pwm_channel *pwm0_ch0_relay; /* relay */
	struct pwm *pwm1; /* control pilot */
	struct pwm_channel *pwm1_ch1_cp; /* control pilot */
	struct pwm *pwm2; /* buzzer */
	struct pwm_channel *pwm2_ch2_buzzer; /* buzzer */
	struct uart *uart1; /* metering */
#if defined(ENABLE_DEBUG_UART0)
	struct uart *uart0;
#endif

	struct gpio *qca7005_cs;
	struct gpio *w5500_cs;
	struct gpio *adc_cs;
	struct gpio *output_power;
	struct gpio *input_power;

	struct gpio *io_expander_reset;
	struct gpio *io_expander_int;
	struct gpio *w5500_int;
	struct gpio *qca7005_int;
	struct gpio *metering_int;

	struct gpio *debug_button;
};

int pinmap_init(struct pinmap_periph *periph);

#if defined(__cplusplus)
}
#endif

#endif /* PINMAP_H */
