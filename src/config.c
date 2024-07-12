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

#include "config.h"

#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "libmcu/metrics.h"

#define NAMESPACE		"config"

struct config {
	struct kvstore *nvs;
};

static struct config cfg;

static const char *cfgstr_tbl[CONFIG_KEY_MAX] = {
	/* Do not exceed 15 characters.          "123456789012345" */
	[CONFIG_KEY_DEVICE_ID]                 = "dev/id",
	[CONFIG_KEY_DEVICE_NAME]               = "dev/name",
	[CONFIG_KEY_DEVICE_MODE]               = "dev/mode",
	[CONFIG_KEY_DEVICE_LOG_MODE]           = "dev/logmod",
	[CONFIG_KEY_DEVICE_LOG_LEVEL]          = "dev/loglvl",
	[CONFIG_KEY_X509_CA]                   = "x509/ca",
	[CONFIG_KEY_X509_CERT]                 = "x509/cert",
	[CONFIG_KEY_DFU_REBOOT_MANUAL]         = "dfu/rst_manual",
	[CONFIG_KEY_CHARGER_MODE]              = "chg/mode",
	[CONFIG_KEY_CHARGER_PARAM]             = "chg/param",
	[CONFIG_KEY_CHARGER_METERING_1]        = "chg/1/metering",
	[CONFIG_KEY_PILOT_PARAM]               = "chg/61851/cp",
	[CONFIG_KEY_NET_MAC]                   = "net/mac",
	[CONFIG_KEY_NET_HEALTH_CHECK_INTERVAL] = "net/health_chk",
	[CONFIG_KEY_WS_PING_INTERVAL]          = "net/ws/ping",
	[CONFIG_KEY_SERVER_URL]                = "net/srv/url",
	[CONFIG_KEY_SERVER_ID]                 = "net/srv/id",
	[CONFIG_KEY_SERVER_PASS]               = "net/srv/pass",
	[CONFIG_KEY_PLC_MAC]                   = "net/plc/mac",
	[CONFIG_KEY_OCPP_PARAM]                = "ocpp/param",
	[CONFIG_KEY_OCPP_CONFIG]               = "ocpp/config",
	/* Do not exceed 15 characters.          "123456789012345" */
};

static const char *get_keystr_from_key(config_key_t key)
{
	if (key < CONFIG_KEY_MAX) {
		return cfgstr_tbl[key];
	}
	return NULL;
}

static bool is_readonly(config_key_t key)
{
	switch (key) {
	case CONFIG_KEY_DEVICE_ID:
		return true;
	default:
		return false;
	}
}

int config_read(config_key_t key, void *buf, size_t bufsize)
{
	const char *keystr = get_keystr_from_key(key);

	if (!keystr) {
		return -ENOENT;
	}

	return kvstore_read(cfg.nvs, keystr, buf, bufsize);
}

int config_save(config_key_t key, const void *data, size_t datasize)
{
	const char *keystr = get_keystr_from_key(key);

	if (!keystr) {
		return -ENOENT;
	}

	if (is_readonly(key)) {
		return -EPERM;
	}

	return kvstore_write(cfg.nvs, keystr, data, datasize);
}

int config_delete(config_key_t key)
{
	const char *keystr = get_keystr_from_key(key);

	if (!keystr) {
		return -ENOENT;
	}

	if (is_readonly(key)) {
		return -EPERM;
	}

	return kvstore_clear(cfg.nvs, keystr);
}

config_key_t config_get_key(const char *keystr)
{
	for (int i = 0; i < CONFIG_KEY_MAX; i++) {
		if (strcmp(keystr, cfgstr_tbl[i]) == 0) {
			return (config_key_t)i;
		}
	}

	return CONFIG_KEY_MAX;
}

const char *config_get_keystr(config_key_t key)
{
	return get_keystr_from_key(key);
}

int config_reset(config_key_t key)
{
	const char *keystr = get_keystr_from_key(key);

	if (!keystr) {
		return -ENOENT;
	}

	if (is_readonly(key)) {
		return -EPERM;
	}

	return kvstore_clear(cfg.nvs, keystr);
}

int config_init(struct kvstore *nvs)
{
	memset(&cfg, 0, sizeof(cfg));

	int err = kvstore_open(nvs, NAMESPACE);
	cfg.nvs = nvs;

	if (err < 0) {
		metrics_increase(ConfigInitError);
	}

	return err;
}
