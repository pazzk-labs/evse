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

#include "libmcu/kvstore.h"
#include "libmcu/metrics.h"
#include "libmcu/crc32.h"

#include "logger.h"

#define NAMESPACE			"config"
#define BASIC_CONFIG_KEY		"basic"

#define CONFIG_ENTRY(key, field, ro)	{ key, offsetof(struct config, field), \
		sizeof(((struct config *)0)->field), ro }

#if !defined(ARRAY_COUNT)
#define ARRAY_COUNT(x)			(sizeof(x) / sizeof((x)[0]))
#endif

typedef enum {
	R, /* read-only */
	RFW, /* read-only but reset to factory default permitted */
	W, /* write-only */
	RW, /* read-write */
} readonly_config_t;

struct config_entry {
	const char *key;
	const size_t offset;
	const size_t size;
	const readonly_config_t permission;
};

struct config_custom_entry {
	const char *key;
	const size_t size;
};

struct config_mgr {
	struct kvstore *nvs;
	struct config basic;
	config_save_cb_t save_cb;
	void *save_cb_ctx;
	bool dirty;
};

typedef void (*iterate_cb_t)(const struct config_entry *entry, void *ctx);
typedef void (*default_handler_t)(struct config *cfg);

static const struct config_entry config_map[] = {
	CONFIG_ENTRY("version",             version,                   R),
	CONFIG_ENTRY("crc",                 crc,                       R),
	CONFIG_ENTRY("device.id",           device_id,                 R),
	CONFIG_ENTRY("device.name",         device_name,               RW),
	CONFIG_ENTRY("device.mode",         device_mode,               RW),
	CONFIG_ENTRY("log.mode",            log_mode,                  RW),
	CONFIG_ENTRY("log.level",           log_level,                 RW),
	CONFIG_ENTRY("dfu.reboot_manually", dfu_reboot_manually,       RW),

	CONFIG_ENTRY("chg.mode",        charger.mode,                  RW),
	CONFIG_ENTRY("chg.param",       charger.param,                 RW),
	CONFIG_ENTRY("chg.count",       charger.connector_count,       RW),
	CONFIG_ENTRY("chg.c1.cp",       charger.connector[0].pilot,    RW),
	CONFIG_ENTRY("chg.c1.metering", charger.connector[0].metering, RW),
	CONFIG_ENTRY("chg.c1.plc_mac",  charger.connector[0].plc_mac,  RW),

	CONFIG_ENTRY("net.mac",         net.mac,                       RW),
	CONFIG_ENTRY("net.health",      net.health_check_interval,     RW),
	CONFIG_ENTRY("net.server.url",  net.server_url,                RW),
	CONFIG_ENTRY("net.server.id",   net.server_id,                 RW),
	CONFIG_ENTRY("net.server.pass", net.server_pass,               RW),
	CONFIG_ENTRY("net.server.ping", net.ping_interval,             RW),

	CONFIG_ENTRY("ocpp.version",    ocpp.version,                  RW),
	CONFIG_ENTRY("ocpp.config",     ocpp.config,                   RW),
	CONFIG_ENTRY("ocpp.checkpoint", ocpp.checkpoint,               RW),
	CONFIG_ENTRY("ocpp.vendor",     ocpp.vendor,                   RFW),
	CONFIG_ENTRY("ocpp.model",      ocpp.model,                    RFW),
};

/* This custom configuration is not part of the basic configuration. Each
 * configuration is stored in a separate key-value pair. */
static const struct config_custom_entry custom_config_map[] = {
	/* Do not exceed 15 characters.
	  "123456789012345" */
	{ "x509.ca",         CONFIG_X509_MAXLEN }, /* CA certificates */
	{ "x509.cert",       CONFIG_X509_MAXLEN }, /* Device certificate */
};

static struct config_mgr mgr;

static void mark_dirty(void)
{
	mgr.dirty = true;
}

static void clear_dirty(void)
{
	mgr.dirty = false;
}

static bool is_dirty(void)
{
	return mgr.dirty;
}

static const struct config_entry *find_config_entry(const char *key)
{
	for (size_t i = 0; i < ARRAY_COUNT(config_map); i++) {
		if (strcmp(config_map[i].key, key) == 0) {
			return &config_map[i];
		}
	}
	return NULL;
}

static const struct config_custom_entry *
find_custom_config_entry(const char *key)
{
	for (size_t i = 0; i < ARRAY_COUNT(custom_config_map); i++) {
		if (strcmp(custom_config_map[i].key, key) == 0) {
			return &custom_config_map[i];
		}
	}
	return NULL;
}

static uint32_t compute_crc(const struct config *cfg)
{
	return crc32_cksum((const uint8_t *)cfg,
			sizeof(*cfg) - sizeof(cfg->crc));
}

static int read_custom_config(const char *key, void *buf, size_t bufsize)
{
	const struct config_custom_entry *entry = find_custom_config_entry(key);

	if (!entry) {
		metrics_increase(ConfigNotFoundCount);
		return -ENOENT;
	}

	if (bufsize < entry->size) {
		metrics_increase(ConfigSizeMismatchCount);
		return -ENOMEM;
	}

	if (kvstore_read(mgr.nvs, key, buf, entry->size) < 0) {
		metrics_increase(ConfigReadErrorCount);
		return -EIO;
	}

	return 0;
}

static int write_custom_config(const char *key,
		const void *data, size_t datasize)
{
	const struct config_custom_entry *entry = find_custom_config_entry(key);

	if (!entry) {
		metrics_increase(ConfigNotFoundCount);
		return -ENOENT;
	}

	if (datasize > entry->size) {
		metrics_increase(ConfigSizeMismatchCount);
		return -ENOMEM;
	}

	if (kvstore_write(mgr.nvs, key, data, datasize) < 0) {
		metrics_increase(ConfigSaveErrorCount);
		return -EIO;
	}

	return 0;
}

static int save_basic_config(struct config *cfg)
{
	if (!is_dirty()) {
		return 0;
	}

	cfg->crc = compute_crc(cfg);

	if (kvstore_write(mgr.nvs, BASIC_CONFIG_KEY, cfg, sizeof(*cfg)) < 0) {
		metrics_increase(ConfigSaveErrorCount);
		return -EIO;
	}

	clear_dirty();

	if (mgr.save_cb) {
		(*mgr.save_cb)(mgr.save_cb_ctx);
	}

	return 0;
}

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

static bool set_default(const char *key, struct config *cfg)
{
	for (size_t i = 0; i < ARRAY_COUNT(default_handlers); i++) {
		if (strcmp(default_handlers[i].key, key) == 0) {
			(*default_handlers[i].handler)(cfg);
			return true;
		}
	}
	return false;
}

static void set_default_or_zero(const struct config_entry *entry,
		struct config *cfg)
{
	if (entry->permission < RFW) {
		return;
	}

	if (!set_default(entry->key, cfg)) {
		memset((uint8_t *)cfg + entry->offset, 0, entry->size);
	}
}

static void on_each_config_reset(const struct config_entry *entry, void *ctx)
{
	set_default_or_zero(entry, (struct config *)ctx);
}

static void iterate_config_entries(iterate_cb_t cb, void *cb_ctx)
{
	for (size_t i = 0; i < ARRAY_COUNT(config_map); i++) {
		const struct config_entry *entry = &config_map[i];
		cb(entry, cb_ctx);
	}
}

static void apply_defaults(struct config *cfg)
{
	iterate_config_entries(on_each_config_reset, cfg);
	cfg->version = CONFIG_VERSION;
	metrics_increase(ConfigDefaultResetCount);
}

static void clear_custom_configs(void)
{
	for (size_t i = 0; i < ARRAY_COUNT(custom_config_map); i++) {
		kvstore_clear(mgr.nvs, custom_config_map[i].key);
	}
}

static bool apply_default(struct config *cfg, const char *key)
{
	if (key == NULL) {
		apply_defaults(cfg);
		clear_custom_configs();
		return true;
	}

	const struct config_entry *entry = find_config_entry(key);

	if (entry) {
		set_default_or_zero(entry, cfg);
		return true;
	}

	const struct config_custom_entry *custom_entry =
		find_custom_config_entry(key);

	if (custom_entry) {
		kvstore_clear(mgr.nvs, key);
		return true;
	}

	error("Key not found: %s", key);

	return false;
}

static void migrate_config(struct config *cfg)
{
	if (cfg->version == CONFIG_VERSION) {
		return;
	} else if (cfg->version > CONFIG_VERSION) {
		/* Downgrade is not supported. It might be a corrupted data. */
		apply_defaults(cfg);
		warn("Downgrading the configuration is not supported.");
	}

	if (cfg->version < MAKE_VERSION(0, 0, 1)) {
		/* v0.0.1 is the first version released. */
		apply_defaults(cfg);
		warn("Migrating the configuration to v0.0.1.");
	}

	cfg->version = CONFIG_VERSION;
	mark_dirty();

	if (save_basic_config(cfg) == 0) {
		metrics_increase(ConfigMigrationCount);
		info("Configuration migrated to v%d.%d.%d",
				GET_VERSION_MAJOR(CONFIG_VERSION),
				GET_VERSION_MINOR(CONFIG_VERSION),
				GET_VERSION_PATCH(CONFIG_VERSION));
	}
}

static void load_config(struct config *cfg)
{
	int err;

	if ((err = kvstore_read(mgr.nvs, BASIC_CONFIG_KEY, cfg, sizeof(*cfg)))
			< 0) {
		apply_defaults(cfg);
                /* Do not mark dirty and give it a chance to recover when
                 * rebooted. */
                warn("Failed to load. Applying defaults.");
	}

	if (cfg->crc != compute_crc(cfg)) {
		apply_defaults(cfg);
		if (!err) { /* flash corruption */
			mark_dirty();
		}
		warn("CRC mismatch detected. Applying defaults.");
	}

	migrate_config(cfg);
}

bool config_is_zeroed(const char *key)
{
	const uint8_t *cfg = (const uint8_t *)&mgr.basic;
	const struct config_entry *entry = find_config_entry(key);

	if (!entry) {
		return false;
	}

	uint8_t zero[entry->size];
	memset(zero, 0, entry->size);

	return memcmp(cfg + entry->offset, zero, entry->size) == 0;
}

int config_get(const char *key, void *buf, size_t bufsize)
{
	const struct config_entry *entry = find_config_entry(key);

	if (!entry) {
		return read_custom_config(key, buf, bufsize);
	}

	if (bufsize < entry->size) {
		metrics_increase(ConfigSizeMismatchCount);
		error("Data size mismatch: %zu < %zu", bufsize, entry->size);
		return -ENOMEM;
	}

	memcpy(buf, (uint8_t *)&mgr.basic + entry->offset, entry->size);

	return 0;
}

int config_set(const char *key, const void *data, size_t datasize)
{
	const struct config_entry *entry = find_config_entry(key);

	if (!entry) {
		return write_custom_config(key, data, datasize);
	}

	if (entry->permission <= RFW) {
		error("Read-only key: %s", key);
		return -EPERM;
	}

	if (datasize > entry->size) {
		metrics_increase(ConfigSizeMismatchCount);
		error("Data size mismatch: %zu > %zu", datasize, entry->size);
		return -ENOMEM;
	}

	if (memcmp((uint8_t *)&mgr.basic + entry->offset, data, datasize)) {
		memcpy((uint8_t *)&mgr.basic + entry->offset, data, datasize);
		mark_dirty();
	}

	return 0;
}

int config_read_all(struct config *cfg)
{
	memcpy(cfg, &mgr.basic, sizeof(*cfg));
	return 0;
}

int config_write_all(const struct config *cfg)
{
	memcpy(&mgr.basic, cfg, sizeof(*cfg));
	mark_dirty();
	return 0;
}

int config_save(void)
{
	return save_basic_config(&mgr.basic);
}

int config_reset(const char *key)
{
	if (apply_default(&mgr.basic, key)) {
		mark_dirty();
		return 0;
	}

	return -ENOENT;
}

int config_update_json(const char *json, size_t json_len)
{
	return -ENOTSUP;
}

int config_init(struct kvstore *nvs, config_save_cb_t cb, void *cb_ctx)
{
	memset(&mgr, 0, sizeof(mgr));

	int err = kvstore_open(nvs, NAMESPACE);
	mgr.nvs = nvs;
	mgr.save_cb = cb;
	mgr.save_cb_ctx = cb_ctx;

	if (err < 0) {
		metrics_increase(ConfigInitError);
		error("Failed to open the configuration namespace: %d", err);
	}

	load_config(&mgr.basic);

	return err;
}
