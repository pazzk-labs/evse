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

#include "config_default.h"
#include "config.h"
#include <string.h>
#include "logger.h"

#if !defined(ARRAY_COUNT)
#define ARRAY_COUNT(x)		(sizeof(x) / sizeof((x)[0]))
#endif

typedef void (*default_handler_t)(struct config *cfg);

static void set_default_chg_mode(struct config *cfg)
{
	strcpy(cfg->charger.mode, "free");
}

static void set_default_chg_c_cnt(struct config *cfg)
{
	cfg->charger.connector_count = 1;
}

static void set_default_chg_c1_plc_mac(struct config *cfg)
{
	cfg->charger.connector[0].plc_mac[0] = 0x02;
	cfg->charger.connector[0].plc_mac[3] = 0xfe;
	cfg->charger.connector[0].plc_mac[4] = 0xed;
}

static void set_default_net_mac(struct config *cfg)
{
	cfg->net.mac[0] = 0x00;
	cfg->net.mac[1] = 0xf2;
}

static void set_default_net_healthcheck(struct config *cfg)
{
	cfg->net.health_check_interval = 60000; /* milliseconds */
}

static void set_default_ping_interval(struct config *cfg)
{
	cfg->net.ping_interval = 120; /* seconds */
}

static void set_default_server_url(struct config *cfg)
{
	strcpy(cfg->net.server_url, "wss://csms.pazzk.net");
}

static void set_default_vendor(struct config *cfg)
{
	strcpy(cfg->ocpp.vendor, "net.pazzk");
}

static void set_default_model(struct config *cfg)
{
	strcpy(cfg->ocpp.model, "EVSE-7S");
}

static const struct {
	const char *key;
	default_handler_t handler;
} default_handlers[] = {
	{ "chg.mode",        set_default_chg_mode },
	{ "chg.count",       set_default_chg_c_cnt },
	{ "chg.c1.plc_mac",  set_default_chg_c1_plc_mac },
	{ "net.mac",         set_default_net_mac },
	{ "net.health",      set_default_net_healthcheck },
	{ "net.server.ping", set_default_ping_interval },
	{ "net.server.url",  set_default_server_url },
	{ "ocpp.vendor",     set_default_vendor },
	{ "ocpp.model",      set_default_model },
};

bool config_set_default(const char *key, struct config *cfg)
{
	for (size_t i = 0; i < ARRAY_COUNT(default_handlers); i++) {
		if (strcmp(default_handlers[i].key, key) == 0) {
			(*default_handlers[i].handler)(cfg);
			return true;
		}
	}
	return false;
}
