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

#ifndef PINMAP_GPIO_H
#define PINMAP_GPIO_H

#if defined(__cplusplus)
extern "C" {
#endif

#define PINMAP_DEBUG_BUTTON		0

#define PINMAP_I2C0_SDA			2
#define PINMAP_I2C0_SCL			1

#define PINMAP_SPI2_MOSI		11
#define PINMAP_SPI2_MISO		13
#define PINMAP_SPI2_SCLK		12
#define PINMAP_SPI2_CS_QCA7005		10
#define PINMAP_SPI2_CS_W5500		9

#define PINMAP_SPI3_MOSI		40
#define PINMAP_SPI3_MISO		41
#define PINMAP_SPI3_SCLK		39
#define PINMAP_SPI3_CS_ADC		42

#define PINMAP_IO_EXPANDER_INT		44
#define PINMAP_IO_EXPANDER_RESET	14

#define PINMAP_UART0_RXD		36 /* For debug */
#define PINMAP_UART0_TXD		37
#define PINMAP_UART1_RXD		17
#define PINMAP_UART1_TXD		16

#define PINMAP_BOARD_REVISION		4 /* ADC1_CH3 */

#define PINMAP_RELAY			46
#define PINMAP_CONTROL_PILOT		7
#define PINMAP_BUZZER			38

#define PINMAP_INPUT_POWER		5 /* ADC1_CH4 */
#define PINMAP_OUTPUT_POWER		15 /* ADC2_CH4, for relay stuck check */

#define PINMAP_W5500_INT		43
#define PINMAP_QCA7005_INT		18
#define PINMAP_METERING_INT		6

#if defined(__cplusplus)
}
#endif

#endif /* PINMAP_GPIO_H */
