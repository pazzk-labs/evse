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

#include "libmcu/gpio.h"
#include "libmcu/i2c.h"
#include "libmcu/spi.h"
#include "libmcu/uart.h"
#include "libmcu/pwm.h"
#include "libmcu/adc.h"

int pinmap_init(struct pinmap_periph *periph)
{
	periph->i2c0 = lm_i2c_create(0, &(struct lm_i2c_pin) {
		.sda = PINMAP_I2C0_SDA,
		.scl = PINMAP_I2C0_SCL,
	});

	periph->spi2 = lm_spi_create(2, &(struct lm_spi_pin) {
		.miso = PINMAP_SPI2_MISO,
		.mosi = PINMAP_SPI2_MOSI,
		.sclk = PINMAP_SPI2_SCLK,
	});
	periph->w5500 = lm_spi_create_device(periph->spi2,
			LM_SPI_MODE_3, 10000000, PINMAP_SPI2_CS_W5500);
	periph->qca7005 = lm_spi_create_device(periph->spi2,
			LM_SPI_MODE_3, 10000000, PINMAP_SPI2_CS_QCA7005);
	periph->spi3 = lm_spi_create(3, &(struct lm_spi_pin) {
		.miso = PINMAP_SPI3_MISO,
		.mosi = PINMAP_SPI3_MOSI,
		.sclk = PINMAP_SPI3_SCLK,
	});
	periph->cpadc = lm_spi_create_device(periph->spi3,
			LM_SPI_MODE_0, 8000000, PINMAP_SPI3_CS_ADC);

	periph->pwm0 = lm_pwm_create(0);
	periph->pwm0_ch0_relay =
		lm_pwm_create_channel(periph->pwm0, 0, PINMAP_RELAY);
	periph->pwm1 = lm_pwm_create(1);
	periph->pwm1_ch1_cp =
		lm_pwm_create_channel(periph->pwm1, 1, PINMAP_CONTROL_PILOT);
	periph->pwm2 = lm_pwm_create(2);
	periph->pwm2_ch2_buzzer =
		lm_pwm_create_channel(periph->pwm2, 2, PINMAP_BUZZER);

	periph->uart1 = lm_uart_create(1, &(struct lm_uart_pin) {
		.rx = PINMAP_UART1_RXD,
		.tx = PINMAP_UART1_TXD,
		.rts = LM_GPIO_PIN_UNASSIGNED,
		.cts = LM_GPIO_PIN_UNASSIGNED,
	});
#if defined(ENABLE_DEBUG_UART0)
	periph->lm_uart0 = lm_uart_create(0, &(struct lm_uart_pin) {
		.rx = PINMAP_UART0_RXD,
		.tx = PINMAP_UART0_TXD,
		.rts = LM_GPIO_PIN_UNASSIGNED,
		.cts = LM_GPIO_PIN_UNASSIGNED,
	});
#endif

	/* NOTE: At least 5ms delay needed before reading ADC to charge the
	 * capacitor as we use 100k input resistance. */
	periph->adc1 = lm_adc_create(1);

	periph->qca7005_cs = lm_gpio_create(PINMAP_SPI2_CS_QCA7005);
	periph->w5500_cs = lm_gpio_create(PINMAP_SPI2_CS_W5500);
	periph->adc_cs = lm_gpio_create(PINMAP_SPI3_CS_ADC);

	periph->output_power = lm_gpio_create(PINMAP_OUTPUT_POWER);
	periph->input_power = lm_gpio_create(PINMAP_INPUT_POWER);

	periph->io_expander_reset = lm_gpio_create(PINMAP_IO_EXPANDER_RESET);
	periph->io_expander_int = lm_gpio_create(PINMAP_IO_EXPANDER_INT);
	periph->w5500_int = lm_gpio_create(PINMAP_W5500_INT);
	periph->qca7005_int = lm_gpio_create(PINMAP_QCA7005_INT);
	periph->metering_int = lm_gpio_create(PINMAP_METERING_INT);

	periph->debug_button = lm_gpio_create(PINMAP_DEBUG_BUTTON);

	return 0;
}
