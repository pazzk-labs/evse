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

#include "tca9539_port.h"
#include "libmcu/i2c.h"
#include "libmcu/gpio.h"
#include "libmcu/timext.h"
#include "libmcu/metrics.h"

#define DEFAULT_TIMEOUT_MS	10

static struct lm_i2c_device *dev;
static struct lm_gpio *reset_pin;

int tca9539_port_write(uint8_t reg_addr, const void *data, size_t data_len)
{
	int err = lm_i2c_write_reg(dev, reg_addr, 8,
			data, data_len, DEFAULT_TIMEOUT_MS);
	if (err < 0) {
		metrics_increase(IOExpanderWriteErrorCount);
	}
	return err;
}

int tca9539_port_read(uint8_t reg_addr, void *buf, size_t bufsize)
{
	int err = lm_i2c_read_reg(dev, reg_addr, 8,
			buf, bufsize, DEFAULT_TIMEOUT_MS);
	if (err < 0) {
		metrics_increase(IOExpanderReadErrorCount);
	}
	return err;
}

void tca9539_port_reset(void)
{
	lm_gpio_set(reset_pin, 0);
	/* keep reset low for at least 6ns */
	sleep_ms(1);
	lm_gpio_set(reset_pin, 1);
}

void tca9539_port_init(int id, void *i2c, void *gpio_reset)
{
	(void)id;
	dev = (struct lm_i2c_device *)i2c;
	reset_pin = (struct lm_gpio *)gpio_reset;

	lm_gpio_enable(reset_pin);
	lm_gpio_set(reset_pin, 1);
}
