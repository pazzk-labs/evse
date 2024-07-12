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

#include "metering.h"

#include <string.h>
#include <errno.h>

#include "libmcu/board.h"
#include "libmcu/timext.h"
#include "hlw811x.h"
#include "exio.h"
#include "logger.h"

#define MIN_INTERVAL_MS			1000

struct metering {
	struct metering_api api;
	struct metering_param param;

	struct hlw811x *hlw811x;
	struct uart *uart;

	uint32_t ts_read; /* timestamp of last read */
};

int hlw811x_ll_write(const uint8_t *data, size_t datalen, void *ctx)
{
	struct uart *uart = (struct uart *)ctx;
	return uart_write(uart, data, datalen);
}

int hlw811x_ll_read(uint8_t *buf, size_t bufsize, void *ctx)
{
	struct uart *uart = (struct uart *)ctx;
	return uart_read(uart, buf, bufsize);
}

static int enable(struct metering *self)
{
	if (exio_set_metering_power(true)) {
		return -EIO;
	}

	const uint8_t hfconst[] = { 0x38, 0xa4 }; /* EC: 3200/kWh */
	hlw811x_error_t err = hlw811x_reset(self->hlw811x);
	sleep_ms(60); /* at least 60ms delay is required after reset. */

	err |= hlw811x_write_reg(self->hlw811x, HLW811X_REG_PULSE_FREQ,
			hfconst, sizeof(hfconst));
	hlw811x_set_resistor_ratio(self->hlw811x,
			&(const struct hlw811x_resistor_ratio) {
		.K1_A = 5,
		.K1_B = 1,
		.K2 = 1,
	});
	err |= hlw811x_set_pga(self->hlw811x, &(const struct hlw811x_pga) {
		.A = HLW811X_PGA_GAIN_2,
		.B = HLW811X_PGA_GAIN_2,
		.U = HLW811X_PGA_GAIN_2,
	});

	err |= hlw811x_set_channel_b_mode(self->hlw811x, HLW811X_B_MODE_NORMAL);
	err |= hlw811x_set_active_power_calc_mode(self->hlw811x,
			HLW811X_ACTIVE_POWER_MODE_POS_NEG_ALGEBRAIC);
	err |= hlw811x_set_rms_calc_mode(self->hlw811x, HLW811X_RMS_MODE_AC);
	err |= hlw811x_set_data_update_frequency(self->hlw811x,
			HLW811X_DATA_UPDATE_FREQ_HZ_3_4);
	err |= hlw811x_set_interrupt_mode(self->hlw811x,
			HLW811X_INTR_PULSE_OUT_A, HLW811X_INTR_B_LEAKAGE);
	err |= hlw811x_enable_waveform(self->hlw811x);
	err |= hlw811x_enable_zerocrossing(self->hlw811x);
	err |= hlw811x_enable_power_factor(self->hlw811x);
	err |= hlw811x_enable_b_channel_comparator(self->hlw811x);
	err |= hlw811x_enable_interrupt(self->hlw811x,
			HLW811X_INTR_PULSE_OUT_A | HLW811X_INTR_B_LEAKAGE);
	err |= hlw811x_enable_channel(self->hlw811x, HLW811X_CHANNEL_ALL);
	err |= hlw811x_enable_pulse(self->hlw811x, HLW811X_CHANNEL_ALL);
	err |= hlw811x_disable_energy_clearance(self->hlw811x,
			HLW811X_CHANNEL_ALL);

	err |= hlw811x_select_channel(self->hlw811x, HLW811X_CHANNEL_A);

	if (err) {
		error("hlw8112 %d", err);
	}

	return err? -EIO: 0;
}

static int disable(struct metering *self)
{
	return exio_set_metering_power(false);
}

static int step(struct metering *self)
{
	const uint32_t now = board_get_time_since_boot_ms();
	const uint32_t elapsed = now - self->ts_read;
	hlw811x_error_t err;
	int32_t Wh;

	if (elapsed < MIN_INTERVAL_MS) {
		return -EAGAIN;
	}

	if ((err = hlw811x_get_energy(self->hlw811x, HLW811X_CHANNEL_A, &Wh))
			!= HLW811X_ERROR_NONE) {
		error("can't get energy: %d", err);
		return -EIO;
	}

	self->param.wh += Wh;
	self->ts_read = now;

	return 0;
}

static int get_voltage(struct metering *self, int32_t *millivolt)
{
	if (!millivolt) {
		return -EINVAL;
	}

	int err = hlw811x_get_rms(self->hlw811x, HLW811X_CHANNEL_U, millivolt);

	if (err != HLW811X_ERROR_NONE) {
		error("can't get voltage: %d", err);
		return -EIO;
	}

	return 0;
}

static int get_current(struct metering *self, int32_t *milliamp)
{
	if (!milliamp) {
		return -EINVAL;
	}

	int err = hlw811x_get_rms(self->hlw811x, HLW811X_CHANNEL_A, milliamp);

	if (err != HLW811X_ERROR_NONE) {
		error("can't get current: %d", err);
		return -EIO;
	}

	return 0;
}

static int get_power_factor(struct metering *self, int32_t *centi)
{
	if (!centi) {
		return -EINVAL;
	}

	int err = hlw811x_get_power_factor(self->hlw811x, centi);

	if (err != HLW811X_ERROR_NONE) {
		error("can't get power factor: %d", err);
		return -EIO;
	}

	return 0;
}

static int get_frequency(struct metering *self, int32_t *centihertz)
{
	if (!centihertz) {
		return -EINVAL;
	}

	int err = hlw811x_get_frequency(self->hlw811x, centihertz);

	if (err != HLW811X_ERROR_NONE) {
		error("can't get frequency: %d", err);
		return -EIO;
	}

	return 0;
}

static int get_phase(struct metering *self, int32_t *centidegree, int hz)
{
	if (!centidegree || (hz != 50 && hz != 60)) {
		return -EINVAL;
	}

	hlw811x_line_freq_t freq = (hz == 50)?
		HLW811X_LINE_FREQ_50HZ: HLW811X_LINE_FREQ_60HZ;

	int err = hlw811x_get_phase_angle(self->hlw811x, centidegree, freq);

	if (err != HLW811X_ERROR_NONE) {
		error("can't get phase angle: %d", err);
		return -EIO;
	}

	return 0;
}

static int get_energy(struct metering *self, uint64_t *wh, uint64_t *varh)
{
	if (!wh) {
		return -EINVAL;
	}

	*wh = self->param.wh;

	return 0;
}

static int get_power(struct metering *self, int32_t *watt, int32_t *var)
{
	if (!watt) {
		return -EINVAL;
	}

	int err = hlw811x_get_power(self->hlw811x, HLW811X_CHANNEL_A, watt);

	if (err != HLW811X_ERROR_NONE) {
		error("can't get power: %d", err);
		return -EIO;
	}

	return 0;
}

static void destroy(struct metering *self)
{
	hlw811x_destroy(self->hlw811x);
	self->hlw811x = NULL;
}

static const struct metering_api *get_api(void)
{
	static const struct metering_api api = {
		.destroy = destroy,
		.enable = enable,
		.disable = disable,
		.step = step,
		.get_voltage = get_voltage,
		.get_current = get_current,
		.get_power_factor = get_power_factor,
		.get_frequency = get_frequency,
		.get_phase = get_phase,
		.get_energy = get_energy,
		.get_power = get_power,
	};

	return &api;
}

struct metering *metering_create_hlw8112(const struct metering_param *param,
		struct metering_io *io)
{
	static struct metering metering;

	if (!(metering.hlw811x = hlw811x_create(HLW811X_UART, io->uart))) {
		return NULL;
	}

	memcpy(&metering.param, param, sizeof(metering.param));
	metering.api = *get_api();
	metering.uart = io->uart;

	return &metering;
}
