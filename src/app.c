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

#include "app.h"
#include <string.h>
#include <sys/time.h>
#include "logger.h"

#include "libmcu/cli.h"
#include "libmcu/adc.h"

#include "config.h"
#include "exio.h"
#include "lis2dw12.h"
#include "tmp102.h"
#include "adc122s051.h"
#include "usrinp.h"
#include "net/netmgr.h"
#include "net/eth_w5500.h"

#include "charger/free.h"
#include "charger/ocpp.h"
#include "charger/connector.h"

static struct app *app;
static struct cli cli;

static void on_charger_event(struct charger *charger,
		struct connector *connector,
		const charger_event_t event, void *ctx) {
	info("charger event: %x", event);

	bool param_updated = false;

	if (event & CHARGER_EVENT_PARAM_CHANGED) { /* availability changed */
		param_updated = true;
	}
	if (event & CHARGER_EVENT_BILLING_STARTED) {
		/* save transaction id for power loss recovery */
		ocpp_charger_backup_tid(connector);
		param_updated = true;
	}
	if (event & CHARGER_EVENT_BILLING_ENDED) {
		/* clear transaction id */
		ocpp_charger_clear_backup_tid(connector);
		param_updated = true;
	}

	if (param_updated) {
		config_save(CONFIG_KEY_OCPP_PARAM,
				ocpp_charger_get_param(charger),
				sizeof(struct ocpp_charger_param));
	}

	if (event & CHARGER_EVENT_REBOOT_REQUIRED) {
		bool reboot_manually = false;
		config_read(CONFIG_KEY_DFU_REBOOT_MANUAL,
				&reboot_manually, sizeof(reboot_manually));
		if (!reboot_manually) {
			app_reboot();
		}
	}
}

void app_adjust_time_on_drift(const time_t unixtime, const uint32_t drift)
{
	const time_t now = time(NULL);
	int32_t diff = (int32_t)(unixtime - now);
	diff = (diff > 0) ? diff : -diff;

	if (diff >= drift) {
		struct timeval tv = {
			.tv_sec = unixtime,
			.tv_usec = 0,
		};
		settimeofday(&tv, NULL);
		info("Time adjusted from %ld to %ld\n",
				(int32_t)now, (int32_t)unixtime);
	}
}

void app_reboot(void)
{
	extern void reboot_gracefully(void);
	reboot_gracefully();
}

int app_process(uint32_t *next_period_ms)
{
#define DEFAULT_STEP_INTERVAL_MS	50
	const uint32_t interval_ms = DEFAULT_STEP_INTERVAL_MS;

	cli_step(&cli);

	charger_step(app->charger, next_period_ms);
	if (next_period_ms) {
		*next_period_ms = interval_ms;
	}

	return 0;
}

void app_init(struct app *ctx)
{
	struct pinmap_periph *periph;
	struct pilot_params cp_params;

	app = ctx;
	periph = &app->periph;

	if (config_read(CONFIG_KEY_PILOT_PARAM,
			&cp_params, sizeof(cp_params)) <= 0) {
		pilot_default_params(&cp_params);
	}

	app->relay = relay_create(periph->pwm0_ch0_relay);
	app->pilot = pilot_create(&cp_params,
			adc122s051_create(periph->cpadc,
				app->adc.buffer.tx, sizeof(app->adc.buffer.tx)),
				periph->pwm1_ch1_cp, app->adc.buffer.rx);

	exio_set_sensor_power(true);
	exio_set_metering_power(true);
	exio_set_w5500_reset(true);
	exio_set_qca7005_reset(true);

	spi_enable(periph->cpadc);
	spi_enable(periph->w5500);

	adc_enable(periph->adc1);
	adc_calibrate(periph->adc1);
	adc_channel_init(periph->adc1, ADC_CH_3); /* board revision */

	lis2dw12_init(periph->acc);
	tmp102_init(periph->temp);

	relay_enable(app->relay);
	pilot_enable(app->pilot);

	usrinp_register_event_cb(USRINP_USB_CONNECT, NULL, NULL);
	usrinp_raise(USRINP_USB_CONNECT); /* in case of already connected */

#define CLI_MAX_HISTORY		10U
	static char buf[CLI_CMD_MAXLEN * CLI_MAX_HISTORY];

	DEFINE_CLI_CMD_LIST(commands, config, help, config, info, log, net,
			reboot, metric, xmodem);

	cli_init(&cli, cli_io_create(), buf, sizeof(buf), app);
	cli_register_cmdlist(&cli, commands);

	uint8_t mac[NETIF_MAC_ADDR_LEN];
	config_read(CONFIG_KEY_NET_MAC, mac, sizeof(mac));
	netmgr_register_iface(eth_w5500_create(periph->w5500), 0, mac, NULL);
	netmgr_enable();

	char mode[8] = "free";
	struct charger_param param;
	charger_default_param(&param);
	config_read(CONFIG_KEY_CHARGER_MODE, mode, sizeof(mode));
	config_read(CONFIG_KEY_CHARGER_PARAM, &param, sizeof(param));

	if (strcmp(mode, "ocpp") == 0) {
		struct ocpp_charger_param ocpp = { 0, };
		config_read(CONFIG_KEY_OCPP_PARAM, &ocpp, sizeof(ocpp));

		app->charger = ocpp_charger_create(&(struct charger_param) {
			.max_input_current_mA = param.max_input_current_mA,
			.input_voltage = param.input_voltage,
			.input_frequency = param.input_frequency,
		}, &ocpp);
	} else {
		app->charger = free_charger_create(&(struct charger_param) {
			.max_input_current_mA = param.max_input_current_mA,
			.input_voltage = param.input_voltage,
			.input_frequency = param.input_frequency,
		});
	}

#define METERING_RX_TIMEOUT_MS		1000
#define METERING_UART_BAUDRATE		38400
	uart_configure(app->periph.uart1, &(struct uart_config) {
		.databit = 8,
		.parity = UART_PARITY_EVEN,
		.stopbit = UART_STOPBIT_1,
		.flowctrl = UART_FLOWCTRL_NONE,
		.rx_timeout_ms = METERING_RX_TIMEOUT_MS,
	});
	uart_enable(app->periph.uart1, METERING_UART_BAUDRATE);

	struct metering_param conn1 = { 0, };
	struct metering_io conn1_io = {
		.uart = app->periph.uart1,
	};
	config_read(CONFIG_KEY_CHARGER_METERING_1, &conn1, sizeof(conn1));

	charger_create_connector(app->charger, &(struct connector_param) {
			.max_output_current_mA = param.max_output_current_mA,
			.min_output_current_mA = param.min_output_current_mA,
			.iec61851 = iec61851_create(app->pilot, app->relay),
			.metering = metering_create(METERING_HLW811X, &conn1, &conn1_io),
			.name = "connector1",
			.priority = 1,
	});
	charger_register_event_cb(app->charger, on_charger_event, app);
}
