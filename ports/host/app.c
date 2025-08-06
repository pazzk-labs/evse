/*
 * This file is part of the Pazzk project <https://pazzk.net/>.
 * Copyright (c) 2025 Pazzk <team@pazzk.net>.
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

#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

#include "libmcu/compiler.h"
#include "libmcu/cli.h"

#include "charger/factory.h"
#include "charger/ocpp.h"
#include "charger/ocpp_connector.h"

#include "safety.h"
#include "../../src/safety/emergency_stop_safety.h"

#include "metering.h"
#include "iec61851.h"

#include "ocpp/ocpp.h"
#include "config.h"
#include "uid.h"
#include "logger.h"

static struct {
	struct cli cli;
	struct termios orig_termios;
	struct app *app;
} m;

static void on_charger_event(struct charger *charger, struct connector *c,
		charger_event_t event, void *ctx)
{
	unused(c);
	unused(ctx);
	if (event & OCPP_CHARGER_EVENT_CHECKPOINT_CHANGED) {
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

static void start_charger(struct app *app)
{
	struct charger_param param;
	struct charger_extension *extension;
	app->charger = charger_factory_create(&param, &extension, NULL);
	charger_init(app->charger, &param, extension);
	charger_register_event_cb(app->charger, on_charger_event, app->charger);

	struct safety *safety = safety_create();
	safety_add_and_enable(safety, emergency_stop_safety_create("es"));

	app->pilot = pilot_create(0, 0, 0, 0);

	struct connector_param conn_param = {
		.max_output_current_mA = param.max_output_current_mA,
		.min_output_current_mA = param.min_output_current_mA,
		.input_frequency = param.input_frequency,
		.iec61851 = iec61851_create(app->pilot, app->relay),
		.metering = metering_create(METERING_HLW811X, 0, 0, 0),
		.safety = safety,
		.name = "c1",
		.priority = 0,
	};

	conn_param.cache = uid_store_create(&(const struct uid_store_config) {
		.fs = app->fs,
		.ns = "cache",
		.capacity = 1024,
	});
	conn_param.local_list = uid_store_create(&(const struct uid_store_config) {
		.fs = app->fs,
		.ns = "localList",
		.capacity = 1024,
	});

	struct connector *c = connector_factory_create(&conn_param);
	charger_attach_connector(app->charger, c);
	connector_register_event_cb(c, on_connector_event, app->charger);
	connector_enable(c);
}

static void enable_cli_raw_mode(void)
{
	struct termios raw;

	tcgetattr(STDIN_FILENO, &m.orig_termios);
	raw = m.orig_termios;

	raw.c_lflag &= (tcflag_t)~(ICANON | ECHO | ISIG);

	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void disable_cli_raw_mode(void)
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &m.orig_termios);
}

static void on_exit_cleanup(void)
{
	disable_cli_raw_mode();
}

void app_adjust_time_on_drift(const time_t unixtime, const uint32_t drift)
{
	unused(unixtime);
	unused(drift);
}

void app_reboot(void)
{
	execv(m.app->argv[0], m.app->argv);
}

int app_process(uint32_t *next_period_ms)
{
#define DEFAULT_STEP_INTERVAL_MS	50
	charger_process(m.app->charger);

	if (next_period_ms) {
		*next_period_ms = DEFAULT_STEP_INTERVAL_MS;
	}
	return 0;
}

void app_init(struct app *app)
{
	m.app = app;

	start_charger(app);

#define CLI_MAX_HISTORY		10U
	static char buf[CLI_CMD_MAXLEN * CLI_MAX_HISTORY];

	DEFINE_CLI_CMD_LIST(commands,
			help, exit, reboot, info, log, metric, dbg, config, net,
			xmodem, chg, idtag, ocpp);

	cli_init(&m.cli, cli_io_create(), buf, sizeof(buf), app);
	cli_register_cmdlist(&m.cli, commands);
	cli_start(&m.cli);

	atexit(on_exit_cleanup);
	signal(SIGINT, (void (*)(int))on_exit_cleanup);
	enable_cli_raw_mode();
}
