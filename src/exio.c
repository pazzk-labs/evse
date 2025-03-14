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

#include "exio.h"
#include "tca9539.h"

static uint8_t output_cache;

static int set_output(uint8_t bit, bool on)
{
	if (on) {
		output_cache |= (uint8_t)(1 << bit);
	} else {
		output_cache &= (uint8_t)~(1 << bit);
	}

	return tca9539_write_output_port0(output_cache);
}

int exio_set_metering_power(bool on)
{
	return set_output(TCA9539_P00, !on);
}

int exio_set_sensor_power(bool on)
{
	return set_output(TCA9539_P01, on);
}

int exio_set_audio_power(bool on)
{
	return set_output(TCA9539_P02, on);
}

int exio_set_qca7005_reset(bool on)
{
	return set_output(TCA9539_P03, on);
}

int exio_set_w5500_reset(bool on)
{
	return set_output(TCA9539_P04, on);
}

int exio_set_led(bool on)
{
	return set_output(TCA9539_P07, on);
}

exio_intr_t exio_get_intr_source(void)
{
	uint32_t intr = EXIO_INTR_NONE;
	uint8_t buf = 0;

	tca9539_read_input_port1(&buf);

	if (buf & (1 << TCA9539_P00)) {
		intr |= EXIO_INTR_EMERGENCY;
	}
	if (buf & (1 << TCA9539_P01)) {
		intr |= EXIO_INTR_ACCELEROMETER;
	}
	if (buf & (1 << TCA9539_P02)) {
		intr |= EXIO_INTR_USB_CONNECT;
	}

	return (exio_intr_t)intr;
}

int exio_get_intr_level(exio_intr_t intr)
{
	uint8_t buf = 0;
	int res = 0;
	int err;

	if ((err = tca9539_read_input_port1(&buf))) {
		return err;
	}

	if (intr & EXIO_INTR_EMERGENCY) {
		res |= (buf & (1 << TCA9539_P00));
	}
	if (intr & EXIO_INTR_ACCELEROMETER) {
		res |= (buf & (1 << TCA9539_P01));
	}
	if (intr & EXIO_INTR_USB_CONNECT) {
		res |= (buf & (1 << TCA9539_P02));
	}

	return res;
}

int exio_init(void *i2c, void *gpio_reset)
{
	int err = tca9539_init(&(struct tca9539_deps) {
		.i2c = i2c,
		.reset_pin = gpio_reset,
	});

	if (!err) {
		err = tca9539_read_output_port0(&output_cache);
	}
	if (!err) {
		/* All pins on port0 are output. */
		err = tca9539_write_direction_port0(0x00);
		/* inverse polarity for emergency stop button */
		err |= tca9539_write_polarity_inversion_port1(1 << TCA9539_P00);
	}

	return err;
}
