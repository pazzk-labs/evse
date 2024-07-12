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

#include "pinmap.h"
#include "pinmap_gpio.h"

int pinmap_init(struct pinmap_periph *periph)
{
	periph->i2c0 = i2c_create(0, &(struct i2c_pin) {
		.sda = PINMAP_I2C0_SDA,
		.scl = PINMAP_I2C0_SCL,
	});

	periph->spi2 = spi_create(2, &(struct spi_pin) {
		.miso = PINMAP_SPI2_MISO,
		.mosi = PINMAP_SPI2_MOSI,
		.sclk = PINMAP_SPI2_SCLK,
	});
	periph->w5500 = spi_create_device(periph->spi2,
			SPI_MODE_3, 10000000, PINMAP_SPI2_CS_W5500);
	periph->qca7005 = spi_create_device(periph->spi2,
			SPI_MODE_3, 10000000, PINMAP_SPI2_CS_QCA7005);
	periph->spi3 = spi_create(3, &(struct spi_pin) {
		.miso = PINMAP_SPI3_MISO,
		.mosi = PINMAP_SPI3_MOSI,
		.sclk = PINMAP_SPI3_SCLK,
	});
	periph->cpadc = spi_create_device(periph->spi3,
			SPI_MODE_0, 8000000, PINMAP_SPI3_CS_ADC);

	periph->pwm0 = pwm_create(0);
	periph->pwm0_ch0_relay =
		pwm_create_channel(periph->pwm0, 0, PINMAP_RELAY);
	periph->pwm1 = pwm_create(1);
	periph->pwm1_ch1_cp =
		pwm_create_channel(periph->pwm1, 1, PINMAP_CONTROL_PILOT);
	periph->pwm2 = pwm_create(2);
	periph->pwm2_ch2_buzzer =
		pwm_create_channel(periph->pwm2, 2, PINMAP_BUZZER);

	periph->uart1 = uart_create(1, &(struct uart_pin) {
		.rx = PINMAP_UART1_RXD,
		.tx = PINMAP_UART1_TXD,
		.rts = -1,
		.cts = -1,
	});
#if defined(ENABLE_DEBUG_UART0)
	periph->uart0 = uart_create(0, &(struct uart_pin) {
		.rx = PINMAP_UART0_RXD,
		.tx = PINMAP_UART0_TXD,
		.rts = -1,
		.cts = -1,
	});
#endif

	/* NOTE: At least 5ms delay needed before reading ADC to charge the
	 * capacitor as we use 100k input resistance. */
	periph->adc1 = adc_create(1);

	periph->qca7005_cs = gpio_create(PINMAP_SPI2_CS_QCA7005);
	periph->w5500_cs = gpio_create(PINMAP_SPI2_CS_W5500);
	periph->adc_cs = gpio_create(PINMAP_SPI3_CS_ADC);

	periph->output_power = gpio_create(PINMAP_OUTPUT_POWER);
	periph->input_power = gpio_create(PINMAP_INPUT_POWER);

	periph->io_expander_reset = gpio_create(PINMAP_IO_EXPANDER_RESET);
	periph->io_expander_int = gpio_create(PINMAP_IO_EXPANDER_INT);
	periph->w5500_int = gpio_create(PINMAP_W5500_INT);
	periph->qca7005_int = gpio_create(PINMAP_QCA7005_INT);
	periph->metering_int = gpio_create(PINMAP_METERING_INT);

	periph->debug_button = gpio_create(PINMAP_DEBUG_BUTTON);

	return 0;
}
