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

#ifndef TCA9539_H
#define TCA9539_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef enum {
	TCA9539_P00,
	TCA9539_P01,
	TCA9539_P02,
	TCA9539_P03,
	TCA9539_P04,
	TCA9539_P05,
	TCA9539_P06,
	TCA9539_P07,
	TCA9539_P10,
	TCA9539_P11,
	TCA9539_P12,
	TCA9539_P13,
	TCA9539_P14,
	TCA9539_P15,
	TCA9539_P16,
	TCA9539_P17,
} tca9359_pin_t;

struct tca9539_deps {
	void *i2c;
	void *reset_pin;
	int id;
};

int tca9539_init(struct tca9539_deps *deps);
void tca9539_reset(void);

int tca9539_read_input_port0(uint8_t *buf);
int tca9539_read_input_port1(uint8_t *buf);

int tca9539_write_output_port0(uint8_t value);
int tca9539_write_output_port1(uint8_t value);
int tca9539_read_output_port0(uint8_t *buf);
int tca9539_read_output_port1(uint8_t *buf);

int tca9539_write_polarity_inversion_port0(uint8_t value);
int tca9539_write_polarity_inversion_port1(uint8_t value);
int tca9539_read_polarity_inversion_port0(uint8_t *buf);
int tca9539_read_polarity_inversion_port1(uint8_t *buf);

int tca9539_write_direction_port0(uint8_t value);
int tca9539_write_direction_port1(uint8_t value);
int tca9539_read_direction_port0(uint8_t *buf);
int tca9539_read_direction_port1(uint8_t *buf);

#if defined(__cplusplus)
}
#endif

#endif /* TCA9539_H */
