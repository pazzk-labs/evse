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
#include <stdlib.h>

#include "libmcu/cli.h"
#include "libmcu/adc.h"
#include "libmcu/uart.h"
#include "libmcu/spi.h"
#include "libmcu/assert.h"
#include "libmcu/compiler.h"

#include "config.h"
#include "exio.h"
#include "lis2dw12.h"
#include "tmp102.h"
#include "adc122s051.h"
#include "metering.h"
#include "relay.h"
#include "iec61851.h"
#include "usrinp.h"
#include "net/netmgr.h"
#include "net/eth_w5500.h"

#include "charger/factory.h"
#include "charger/ocpp.h"
#include "charger/ocpp_connector.h"
#include "ocpp/ocpp.h"
#include "logger.h"

static_assert(sizeof(struct metering_energy)
		== sizeof(((struct config *)0)->charger.connector[0].metering),
		"metering_energy size mismatch");
static_assert(sizeof(struct pilot_params)
		== sizeof(((struct config *)0)->charger.connector[0].pilot),
		"pilot_params size mismatch");

static const char *metering_ch1_key = "chg.c1.metering";

static struct {
	struct cli cli;
	struct charger *charger;
} m;

static void on_charger_event(struct charger *charger, struct connector *c,
		charger_event_t event, void *ctx)
{
	unused(c);
	unused(ctx);
	if (event & OCPP_CHARGER_EVENT_AVAILABILITY_CHANGED) {
		/* Availability changes are saved in non-volatile memory to keep
		 * them persistent. */
		const struct ocpp_checkpoint *checkpoint =
			ocpp_charger_get_checkpoint(charger);
		config_set_and_save("ocpp.checkpoint",
				checkpoint, sizeof(*checkpoint));
	}

	if (event & OCPP_CHARGER_EVENT_CONFIGURATION_CHANGED) {
		const size_t len = ocpp_compute_configuration_size();
		void *p = calloc(1, len);
		if (p) {
			ocpp_copy_configuration_to(p, len);
			config_set_and_save("ocpp.config", p, len);
			free(p);
		}
	}

	if (event & OCPP_CHARGER_EVENT_REBOOT_REQUIRED) {
		bool reboot_manually = false;
		config_get("dfu.reboot_manually",
				&reboot_manually, sizeof(reboot_manually));
		if (!reboot_manually) {
			app_reboot();
		}
	}
}

static void on_connector_event(struct connector *self,
		connector_event_t event, void *ctx) {
	char evtstr[CONNECTOR_EVENT_STRING_MAXLEN];
	struct charger *charger = (struct charger *)ctx;

	connector_stringify_event(event, evtstr, sizeof(evtstr));
	info("connector event: \"%s\"", evtstr);

	if (event & CONNECTOR_EVENT_CHARGING_ENDED) {
		struct metering *meter = connector_meter(self);
		if (meter) {
			metering_save_energy(meter);
		}
	}

	/* The following are OCPP-specific events */
	if (!charger_supports(charger, "ocpp")) {
		warn("OCPP is not supported");
		return;
	}

	struct ocpp_checkpoint *checkpoint =
		ocpp_charger_get_checkpoint(charger);

	if (event & CONNECTOR_EVENT_ENABLED) {
		int cid;
		if (charger_get_connector_id(charger, self, &cid) == 0) {
			assert(cid > 0 && cid <= OCPP_CONNECTOR_MAX);
			struct ocpp_connector_checkpoint *ccp =
				&checkpoint->connector[cid-1];
			ocpp_connector_link_checkpoint(self, ccp);
			info("connector%d enabled: \"%s\", missing %u",
					cid, ccp->unavailable?
						"unavailable" : "available",
					ccp->transaction_id);
		}
	}

	if (event & (CONNECTOR_EVENT_BILLING_STARTED |
			CONNECTOR_EVENT_BILLING_ENDED)) {
		/* The transaction ID will be saved or cleared in non-volatile
		 * memory in case of a power failure */
		config_set_and_save("ocpp.checkpoint",
				checkpoint, sizeof(*checkpoint));
	}
}

static bool on_metering_save(const struct metering *metering,
		const struct metering_energy *energy, void *ctx)
{
	unused(metering);

	const char *key = (const char *)ctx;

	if (key && config_set_and_save(key, energy, sizeof(*energy)) == 0) {
		info("metering save: %lluWh to %s", energy->wh, key);
		return true;
	}

	return false;
}

static void setup_charger_components(struct app *app)
{
#define METERING_RX_TIMEOUT_MS		200 /* 768 bytes @ 38400bps */
#define METERING_UART_BAUDRATE		38400
	struct pinmap_periph *periph = &app->periph;
	struct pilot_params cp_params;

	/* FIXME: each connector should have its own cp params */
	pilot_default_params(&cp_params);
	if (!config_is_zeroed("chg.c1.cp")) {
		config_get("chg.c1.cp", &cp_params, sizeof(cp_params));
	}

	app->relay = relay_create(periph->pwm0_ch0_relay);
	app->pilot = pilot_create(&cp_params,
			adc122s051_create(periph->cpadc,
				app->adc.buffer.tx, sizeof(app->adc.buffer.tx)),
				periph->pwm1_ch1_cp, app->adc.buffer.rx);

	lm_uart_configure(app->periph.uart1, &(struct lm_uart_config) {
		.databit = 8,
		.parity = LM_UART_PARITY_EVEN,
		.stopbit = LM_UART_STOPBIT_1,
		.flowctrl = LM_UART_FLOWCTRL_NONE,
		.rx_timeout_ms = METERING_RX_TIMEOUT_MS,
	});

	exio_set_metering_power(true);
	relay_enable(app->relay);
	pilot_enable(app->pilot);
	lm_uart_enable(app->periph.uart1, METERING_UART_BAUDRATE);
}

static void start_charger(struct app *app)
{
	struct charger_param param;
	struct charger_extension *extension;
	m.charger = charger_factory_create(&param, &extension, NULL);
	charger_init(m.charger, &param, extension);
	charger_register_event_cb(m.charger, on_charger_event, &m);

	struct metering_io conn1_io = {
		.uart = app->periph.uart1,
	};
	struct metering_param conn1 = {
		.io = &conn1_io,
	};
	union {
		const char *str;
		void *p;
	} conn1_key = { .str = metering_ch1_key };

	config_get(metering_ch1_key, &conn1.energy, sizeof(conn1.energy));

	struct connector_param conn_param = {
		.max_output_current_mA = param.max_output_current_mA,
		.min_output_current_mA = param.min_output_current_mA,
		.input_frequency = param.input_frequency,
		.iec61851 = iec61851_create(app->pilot, app->relay),
		.metering = metering_create(METERING_HLW811X, &conn1,
				on_metering_save, conn1_key.p),
		.name = "c1",
		.priority = 0,
	};

	struct connector *c = connector_factory_create(&conn_param);
	charger_attach_connector(m.charger, c);
	connector_register_event_cb(c, on_connector_event, m.charger);
	connector_enable(c);
}

void app_adjust_time_on_drift(const time_t unixtime, const uint32_t drift)
{
	const time_t now = time(NULL);
	int32_t diff = (int32_t)(unixtime - now);
	diff = (diff > 0) ? diff : -diff;

	if ((uint32_t)diff >= drift) {
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
	charger_process(m.charger);

	if (next_period_ms) {
		*next_period_ms = DEFAULT_STEP_INTERVAL_MS;
	}

	return 0;
}

void app_init(struct app *app)
{
	struct pinmap_periph *periph = &app->periph;

	exio_set_sensor_power(true);
	exio_set_w5500_reset(true);
	exio_set_qca7005_reset(true);

	lm_spi_enable(periph->cpadc);
	lm_spi_enable(periph->w5500);

	lm_adc_enable(periph->adc1);
	lm_adc_calibrate(periph->adc1);
	lm_adc_channel_init(periph->adc1, LM_ADC_CH_3); /* board revision */

	lis2dw12_init(periph->acc);
	tmp102_init(periph->temp);

	usrinp_register_event_cb(USRINP_USB_CONNECT, NULL, NULL);
	usrinp_raise(USRINP_USB_CONNECT); /* in case of already connected */

#define CLI_MAX_HISTORY		10U
	static char buf[CLI_CMD_MAXLEN * CLI_MAX_HISTORY];

	DEFINE_CLI_CMD_LIST(commands, config, help, info, log, net, ocpp,
			reboot, metric, sec, xmodem);

	cli_init(&m.cli, cli_io_create(), buf, sizeof(buf), app);
	cli_register_cmdlist(&m.cli, commands);
	cli_start(&m.cli);

	uint8_t mac[NETIF_MAC_ADDR_LEN];
	config_get("net.mac", mac, sizeof(mac));
	netmgr_register_iface(eth_w5500_create(periph->w5500), 0, mac, NULL);
	netmgr_enable();

	setup_charger_components(app);
	start_charger(app);

	info("MAC: %02X:%02X:%02X:%02X:%02X:%02X",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
