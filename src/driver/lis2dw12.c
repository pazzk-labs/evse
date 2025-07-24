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

#include "lis2dw12.h"

#include <errno.h>
#include <assert.h>

enum registers {
	OUT_T_L			= 0x0DU,
	OUT_T_H			= 0x0EU,
	WHO_AM_I		= 0x0FU,
	CTRL1			= 0x20U,
	CTRL2			= 0x21U,
	CTRL3			= 0x22U,
	CTRL4_INT1_PAD_CTRL	= 0x23U,
	CTRL5_INT2_PAD_CTRL	= 0x24U,
	CTRL6			= 0x25U,
	OUT_T			= 0x26U,
	STATUS			= 0x27U,
	OUT_X_L			= 0x28U,
	OUT_X_H			= 0x29U,
	OUT_Y_L			= 0x2AU,
	OUT_Y_H			= 0x2BU,
	OUT_Z_L			= 0x2CU,
	OUT_Z_H			= 0x2DU,
	FIFO_CTRL		= 0x2EU,
	FIFO_SAMPLES		= 0x2FU,
	TAP_THS_X		= 0x30U,
	TAP_THS_Y		= 0x31U,
	TAP_THS_Z		= 0x32U,
	INT_DUR			= 0x33U,
	WAKE_UP_THS		= 0x34U,
	WAKE_UP_DUR		= 0x35U,
	FREE_FALL		= 0x36U,
	STATUS_DUP		= 0x37U,
	WAKE_UP_SRC		= 0x38U,
	TAP_SRC			= 0x39U,
	SIXD_SRC		= 0x3AU,
	ALL_INT_SRC		= 0x3BU,
	X_OFS_USR		= 0x3CU,
	Y_OFS_USR		= 0x3DU,
	Z_OFS_USR		= 0x3EU,
	CTRL_REG7		= 0x3FU,
};

static void *user_context;

static int write_reg(uint8_t reg, uint8_t val)
{
	return lis2dw12_port_write_reg(user_context, reg, &val, sizeof(val));
}

static int read_reg(uint8_t reg, uint8_t *p)
{
	return lis2dw12_port_read_reg(user_context, reg, p, sizeof(*p));
}

static int set_reg(uint8_t reg, uint8_t bit, uint8_t mask, uint8_t val)
{
	uint8_t tmp;
	int err;

	if ((err = read_reg(reg, &tmp)) != 0) {
		return err;
	}

	tmp = tmp & (uint8_t)~(mask << bit);
	tmp = tmp | (uint8_t)(val << bit);

	return write_reg(reg, tmp);
}

int lis2dw12_set_ctrl3(uint8_t regval)
{
	return write_reg(CTRL3, regval);
}

int lis2dw12_set_mode(lis2dw12_mode_t mode, lis2dw12_odr_t odr)
{
	uint8_t reg = 0;

	switch (mode) {
	case LIS2DW12_MODE_HP:
		reg |= 1U << 2;
		break;
	case LIS2DW12_MODE_MANUAL:
		reg |= 1U << 3;
		break;
	case LIS2DW12_MODE_LP2:
		reg |= 1U;
		break;
	case LIS2DW12_MODE_LP3:
		reg |= 1U << 1;
		break;
	case LIS2DW12_MODE_LP4:
		reg |= 3U;
		break;
	case LIS2DW12_MODE_LP1: /* fall through */
	default:
		break;
	}

	reg |= (uint8_t)(odr << 4);

	return write_reg(CTRL1, reg);
}

int lis2dw12_status(lis2dw12_status_t *status)
{
	uint8_t reg = 0;
	uint8_t reg_dup = 0;

	int err = read_reg(STATUS, &reg);

	if (!err) {
		err = read_reg(STATUS_DUP, &reg_dup);
	}

	union {
		uint16_t u16;
		lis2dw12_status_t status;
	} t;

	t.u16 = (uint16_t)((uint16_t)reg_dup << 8 | reg);
	*status = t.status;

	return err;
}

bool lis2dw12_data_ready(void)
{
	uint8_t reg = 0;

	if (read_reg(STATUS, &reg) != 0) {
		return false;
	}

	return reg & 1;
}

int lis2dw12_measure_single(void)
{
	return set_reg(CTRL3, 0, 3, 3);
}

int lis2dw12_read_measurement(int16_t *x, int16_t *y, int16_t *z)
{
	uint8_t val[6];
	int err = lis2dw12_port_read_reg(user_context,
			OUT_X_L, val, sizeof(val));
	if (err) {
		return err;
	}

	*x = (int16_t)(val[0] | ((int16_t)val[1] << 8));
	*y = (int16_t)(val[2] | ((int16_t)val[3] << 8));
	*z = (int16_t)(val[4] | ((int16_t)val[5] << 8));

	return 0;
}

int lis2dw12_read_temperature(int16_t *temperature)
{
	uint8_t val[2];
	int err = lis2dw12_port_read_reg(user_context,
			OUT_T_L, val, sizeof(val));
	if (err) {
		return err;
	}

	*temperature = (int16_t)(val[0] | ((int16_t)val[1] << 8));

	return 0;
}

int lis2dw12_enable_interrupt(lis2dw12_event_t interrupts)
{
	uint8_t reg = 0;

	for (int i = 0; i < 8; i++) {
		if (interrupts & (1U << i)) {
			reg |= 1U << i;
		}
	}

	int err = write_reg(CTRL4_INT1_PAD_CTRL, reg);
	if (err) {
		return err;
	}

	return set_reg(CTRL_REG7, 5, 1, 1);
}

lis2dw12_int_src_t lis2dw12_get_interrupt_sources(void)
{
	uint8_t val = 0;
	int err = read_reg(ALL_INT_SRC, &val);
	if (err) {
		return err;
	}

	lis2dw12_int_src_t srcs = LIS2DW12_INT_NONE;

	if (val & 0x01) {
		srcs |= LIS2DW12_INT_FREEFALL;
	}
	if (val & 0x02) {
		srcs |= LIS2DW12_INT_WAKEUP;
	}
	if (val & 0x04) {
		srcs |= LIS2DW12_INT_SINGLE_TAP;
	}
	if (val & 0x08) {
		srcs |= LIS2DW12_INT_DOUBLE_TAP;
	}
	if (val & 0x10) {
		srcs |= LIS2DW12_INT_6D;
	}
	if (val & 0x20) {
		srcs |= LIS2DW12_INT_SLEEP_CHANGE;
	}

	return srcs;
}

int lis2dw12_enable_tap(lis2dw12_axis_t axis)
{
	int err = 0;

	if (axis & LIS2DW12_X) {
		err |= set_reg(TAP_THS_Z, 7, 1, 1);
	}
	if (axis & LIS2DW12_Y) {
		err |= set_reg(TAP_THS_Z, 6, 1, 1);
	}
	if (axis & LIS2DW12_Z) {
		err |= set_reg(TAP_THS_Z, 5, 1, 1);
	}

	return err;
}

int lis2dw12_disable_tap(lis2dw12_axis_t axis)
{
	int err = 0;

	if (axis & LIS2DW12_X) {
		err |= set_reg(TAP_THS_Z, 7, 1, 0);
	}
	if (axis & LIS2DW12_Y) {
		err |= set_reg(TAP_THS_Z, 6, 1, 0);
	}
	if (axis & LIS2DW12_Z) {
		err |= set_reg(TAP_THS_Z, 5, 1, 0);
	}

	return err;
}

int lis2dw12_set_tap_threshold(lis2dw12_axis_t axis, uint8_t val)
{
	int err = 0;

	if (axis & LIS2DW12_X) {
		err |= set_reg(TAP_THS_X, 0, 0x1f, val);
	}
	if (axis & LIS2DW12_Y) {
		err |= set_reg(TAP_THS_Y, 0, 0x1f, val);
	}
	if (axis & LIS2DW12_Z) {
		err |= set_reg(TAP_THS_Z, 0, 0x1f, val);
	}

	set_reg(TAP_THS_Y, 5, 7, 7);
	set_reg(WAKE_UP_THS, 6, 1, 1);

	return err;
}

int lis2dw12_set_full_scale(uint8_t g)
{
	assert(g == 2 || g == 4 || g == 8 || g == 16);
	g = g < 16? g >> 2 : 3;
	return set_reg(CTRL6, 4, 3, g);
}

int lis2dw12_set_bandwidth(uint8_t odr_divider)
{
	return set_reg(CTRL6, 6, 3, odr_divider);
}

int lis2dw12_enable_noise_filter(void)
{
	return set_reg(CTRL6, 2, 1, 1);
}

int lis2dw12_read(uint8_t reg, uint8_t *val)
{
	return read_reg(reg, val);
}

int lis2dw12_whoami(void)
{
	uint8_t whoami = 0;
	int err = read_reg(WHO_AM_I, &whoami);

	if (err) {
		return err;
	}

	return whoami == 0x44? 0 : -EFAULT;
}

bool lis2dw12_reset_busy(void)
{
	uint8_t reg;

	if (read_reg(CTRL2, &reg) != 0) {
		return true;
	}

	return reg & 0x40;
}

int lis2dw12_reset_soft(void)
{
	return set_reg(CTRL2, 6, 1, 1);
}

int lis2dw12_init(void *ctx)
{
	user_context = ctx;
	return 0;
}
