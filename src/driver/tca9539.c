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

#include "tca9539.h"
#include "tca9539_port.h"

enum {
	REG_INPUT_PORT0,
	REG_INPUT_PORT1,
	REG_OUTPUT_PORT0,
	REG_OUTPUT_PORT1,
	REG_POLARITY_INVERSION_PORT0,
	REG_POLARITY_INVERSION_PORT1,
	REG_DIRECTION_PORT0,
	REG_DIRECTION_PORT1,
	REG_MAX,
};

static int read_port(uint8_t reg, uint8_t *buf)
{
	return tca9539_port_read(reg, buf, sizeof(*buf));
}

static int write_port(uint8_t reg, uint8_t value)
{
	return tca9539_port_write(reg, &value, sizeof(value));
}

int tca9539_read_input_port0(uint8_t *buf)
{
	return read_port(REG_INPUT_PORT0, buf);
}

int tca9539_read_input_port1(uint8_t *buf)
{
	return read_port(REG_INPUT_PORT1, buf);
}

int tca9539_write_output_port0(uint8_t value)
{
	return write_port(REG_OUTPUT_PORT0, value);
}

int tca9539_write_output_port1(uint8_t value)
{
	return write_port(REG_OUTPUT_PORT1, value);
}

int tca9539_read_output_port0(uint8_t *buf)
{
	return read_port(REG_OUTPUT_PORT0, buf);
}

int tca9539_read_output_port1(uint8_t *buf)
{
	return read_port(REG_OUTPUT_PORT1, buf);
}

int tca9539_write_polarity_inversion_port0(uint8_t value)
{
	return write_port(REG_POLARITY_INVERSION_PORT0, value);
}

int tca9539_write_polarity_inversion_port1(uint8_t value)
{
	return write_port(REG_POLARITY_INVERSION_PORT1, value);
}

int tca9539_read_polarity_inversion_port0(uint8_t *buf)
{
	return read_port(REG_POLARITY_INVERSION_PORT0, buf);
}

int tca9539_read_polarity_inversion_port1(uint8_t *buf)
{
	return read_port(REG_POLARITY_INVERSION_PORT1, buf);
}

int tca9539_write_direction_port0(uint8_t value)
{
	return write_port(REG_DIRECTION_PORT0, value);
}

int tca9539_write_direction_port1(uint8_t value)
{
	return write_port(REG_DIRECTION_PORT1, value);
}

int tca9539_read_direction_port0(uint8_t *buf)
{
	return read_port(REG_DIRECTION_PORT0, buf);
}

int tca9539_read_direction_port1(uint8_t *buf)
{
	return read_port(REG_DIRECTION_PORT1, buf);
}

void tca9539_reset(void)
{
	tca9539_port_reset();
}

int tca9539_init(struct tca9539_deps *deps)
{
	tca9539_port_init(deps->id, deps->i2c, deps->reset_pin);

	return 0;
}
