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

#ifndef LIBMCU_LIS2DW12_H
#define LIBMCU_LIS2DW12_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum lis2dw12_mode {
	LIS2DW12_MODE_LP1,	/**< Low power mode 1(12-bit) */
	LIS2DW12_MODE_LP2,	/**< Low power mode 2(14-bit) */
	LIS2DW12_MODE_LP3,	/**< Low power mode 3(14-bit) */
	LIS2DW12_MODE_LP4,	/**< Low power mode 4(14-bit) */
	LIS2DW12_MODE_HP,	/**< High performance */
	LIS2DW12_MODE_MANUAL,
} lis2dw12_mode_t;

typedef enum lis2dw12_odr {
	LIS2DW12_ODR_STOP,
	LIS2DW12_ODR_1_6,	/**< 1.6Hz */
	LIS2DW12_ODR_12_5,	/**< 12.5Hz */
	LIS2DW12_ODR_25,	/**< 25Hz */
	LIS2DW12_ODR_50,	/**< 50Hz */
	LIS2DW12_ODR_100,	/**< 100Hz */
	LIS2DW12_ODR_200,	/**< 200Hz */
	LIS2DW12_ODR_400,	/**< 400Hz */
	LIS2DW12_ODR_800,	/**< 800Hz */
	LIS2DW12_ODR_1600,	/**< 1600Hz */
} lis2dw12_odr_t;

typedef struct lis2dw12_status {
	uint16_t data_ready    : 1;
	uint16_t freefall      : 1;
	uint16_t position      : 1;
	uint16_t single_tap    : 1;
	uint16_t double_tap    : 1;
	uint16_t sleep         : 1;
	uint16_t wakeup        : 1;
	uint16_t fifo_overrun  : 1;

	uint16_t data_ready2   : 1;
	uint16_t freefall2     : 1;
	uint16_t position2     : 1;
	uint16_t single_tap2   : 1;
	uint16_t double_tap2   : 1;
	uint16_t sleep2        : 1;
	uint16_t temperature   : 1;
	uint16_t fifo_overrun2 : 1;
} lis2dw12_status_t;

enum lis2dw12_event {
	LIS2DW12_EVT_DATA_READY		= 0x01,
	LIS2DW12_EVT_FIFO_OVERRUN	= 0x02,
	LIS2DW12_EVT_FIFO_FULL		= 0x04,
	LIS2DW12_EVT_DOUBLE_TAP		= 0x08,
	LIS2DW12_EVT_FREEFALL		= 0x10,
	LIS2DW12_EVT_WAKEUP		= 0x20,
	LIS2DW12_EVT_SINGLE_TAP		= 0x40,
	LIS2DW12_EVT_6D			= 0x80,
};
typedef uint8_t lis2dw12_event_t;

enum lis2dw12_axis {
	LIS2DW12_X			= 0x01,
	LIS2DW12_Y			= 0x02,
	LIS2DW12_Z			= 0x04,
};
typedef uint8_t lis2dw12_axis_t;

enum lis2dw12_interrupt_source {
	LIS2DW12_INT_NONE		= 0x00,
	LIS2DW12_INT_SLEEP_CHANGE	= 0x01,
	LIS2DW12_INT_6D			= 0x02,
	LIS2DW12_INT_DOUBLE_TAP		= 0x04,
	LIS2DW12_INT_SINGLE_TAP		= 0x08,
	LIS2DW12_INT_WAKEUP		= 0x10,
	LIS2DW12_INT_FREEFALL		= 0x20,
};
typedef int lis2dw12_int_src_t;

int lis2dw12_init(void *ctx);

int lis2dw12_reset_soft(void);
bool lis2dw12_reset_busy(void);
int lis2dw12_whoami(void);
int lis2dw12_status(lis2dw12_status_t *status);
int lis2dw12_set_mode(lis2dw12_mode_t mode, lis2dw12_odr_t odr);
int lis2dw12_measure_single(void);
bool lis2dw12_data_ready(void);
int lis2dw12_read_measurement(int16_t *x, int16_t *y, int16_t *z);
int lis2dw12_enable_interrupt(lis2dw12_event_t interrupts);
lis2dw12_int_src_t lis2dw12_get_interrupt_sources(void);
int lis2dw12_read_temperature(int16_t *temperature);
int lis2dw12_enable_tap(lis2dw12_axis_t axis);
int lis2dw12_disable_tap(lis2dw12_axis_t axis);
int lis2dw12_set_tap_threshold(lis2dw12_axis_t axis, uint8_t val);
int lis2dw12_enable_noise_filter(void);
int lis2dw12_set_full_scale(uint8_t g);
int lis2dw12_set_bandwidth(uint8_t odr_divider);
int lis2dw12_set_ctrl3(uint8_t regval);
int lis2dw12_read(uint8_t reg, uint8_t *val);

int lis2dw12_port_write_reg(void *ctx,
		uint8_t reg, const uint8_t *data, size_t datasize);
int lis2dw12_port_read_reg(void *ctx,
		uint8_t reg, uint8_t *buf, size_t bufsize);

#if defined(__cplusplus)
}
#endif

#endif /* LIBMCU_LIS2DW12_H */
