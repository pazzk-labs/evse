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

#ifndef CONFIG_H
#define CONFIG_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "libmcu/compiler.h"
#include "version.h"

#define CONFIG_VERSION				MAKE_VERSION(0, 0, 1)

#define CONFIG_DEVICE_ID_MAXLEN			24
#define CONFIG_DEVICE_NAME_MAXLEN		32
#define CONFIG_CHARGER_MODE_MAXLEN		8
#define CONFIG_CHARGER_CONNECTOR_MAX		1
#define CONFIG_X509_MAXLEN			2048

typedef void (*config_save_cb_t)(void *ctx);

struct config_charger {
	char mode[CONFIG_CHARGER_MODE_MAXLEN];
	uint8_t param[16];
	uint8_t connector_count;
	struct {
		uint8_t metering[16];
		uint8_t pilot[30];
		uint8_t plc_mac[6];
	} connector[CONFIG_CHARGER_CONNECTOR_MAX];
} LIBMCU_PACKED;

struct config_net {
	uint8_t mac[6];
	uint32_t health_check_interval;
	uint32_t ping_interval;
	char server_url[256];
	char server_id[32];
	char server_pass[40];
} LIBMCU_PACKED;

struct config_ocpp {
	uint32_t version;
	uint8_t config[546];
	uint8_t checkpoint[16];
	char vendor[20+1];
	char model[20+1];
} LIBMCU_PACKED;

struct config {
	uint32_t version;

	char device_id[CONFIG_DEVICE_ID_MAXLEN];
	char device_name[CONFIG_DEVICE_NAME_MAXLEN];
	uint8_t device_mode;

	uint8_t log_mode;
	uint8_t log_level;

	bool dfu_reboot_manually;

	struct config_charger charger;
	struct config_net net;
	struct config_ocpp ocpp;

	uint32_t crc; /* keep this field at the end */
} LIBMCU_PACKED;
static_assert(sizeof(struct config) == 1095, "config size mismatch");

struct kvstore;

/**
 * @brief Initializes the configuration module.
 * @param[in] nvs Handle to the non-volatile storage.
 * @param[in] cb Callback function for configuration save events.
 * @param[in] cb_ctx Context to be passed to the callback function.
 * @return 0 on success, negative error code on failure.
 */
int config_init(struct kvstore *nvs, config_save_cb_t cb, void *cb_ctx);

/**
 * @brief Reads a configuration value.
 * @param[in] key Configuration key string (e.g., "device.id").
 * @param[out] buf Buffer to store the retrieved value.
 * @param[in] bufsize Size of the buffer.
 * @return 0 on success, negative error code on failure.
 */
int config_get(const char *key, void *buf, size_t bufsize);

/**
 * @brief Writes a configuration value.
 * @param[in] key Configuration key string (e.g., "device.id").
 * @param[in] data Data to be written.
 * @param[in] datasize Size of the data to be written.
 * @return 0 on success, negative error code on failure.
 */
int config_set(const char *key, const void *data, size_t datasize);

/**
 * @brief Reads all configuration settings.
 * @param[out] cfg A pointer to the config structure where the settings will be
 *             stored.
 * @return int Returns 0 on success, or a negative error code on failure.
 */
int config_read_all(struct config *cfg);

/**
 * @brief Writes all configuration settings.
 * @param cfg A pointer to the config structure containing the settings to be
 *        written.
 * @return int Returns 0 on success, or a negative error code on failure.
 */
int config_write_all(const struct config *cfg);

/**
 * @brief Saves the modified configuration values.
 * @return 0 on success, negative error code on failure.
 */
int config_save(void);

/**
 * @brief Writes a configuration value and saves it.
 *
 * This function sets the configuration value associated with the given key
 * and saves the configuration to persistent storage.
 *
 * @param[in] key Configuration key string (e.g., "device.id").
 * @param[in] data Data to be written.
 * @param[in] datasize Size of the data to be written.
 *
 * @return 0 on success, negative error code on failure.
 */
int config_set_and_save(const char *key, const void *data, size_t datasize);

/**
 * @brief Resets a specific configuration key to its default value.
 * @param[in] key Configuration key to reset (NULL resets all configurations).
 * @return 0 on success, negative error code on failure.
 */
int config_reset(const char *key);

/**
 * @brief Updates configuration values using a JSON string.
 * @param[in] json JSON-formatted configuration string.
 * @param[in] json_len Length of the JSON string.
 * @return 0 on success, negative error code on failure.
 */
int config_update_json(const char *json, size_t json_len);

/**
 * @brief Checks if the configuration setting for the given key is zeroed.
 *
 * This function does not support custom settings. If the key corresponds to a
 * custom setting, the function will return false.
 *
 * @warning Be cautious when using this function to determine initialization
 *          status, as the actual configuration value might be zero.
 *
 * @param[in] key The key of the configuration setting to check.
 *
 * @return bool Returns true if the configuration setting is zeroed, false
 *         otherwise.
 */
bool config_is_zeroed(const char *key);

#if defined(__cplusplus)
}
#endif

#endif /* CONFIG_H */
