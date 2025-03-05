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

#include "libmcu/kvstore.h"
#include <stddef.h>

typedef enum {
	CONFIG_KEY_DEVICE_ID,
	CONFIG_KEY_DEVICE_NAME,
	CONFIG_KEY_DEVICE_MODE,
	CONFIG_KEY_DEVICE_LOG_MODE,
	CONFIG_KEY_DEVICE_LOG_LEVEL,
	CONFIG_KEY_X509_CA, /* CA certificate chain */
	CONFIG_KEY_X509_CERT, /* device certificate */
	CONFIG_KEY_DFU_REBOOT_MANUAL,
	CONFIG_KEY_CHARGER_MODE,
	CONFIG_KEY_CHARGER_PARAM,
	CONFIG_KEY_CHARGER_METERING_1,
	CONFIG_KEY_PILOT_PARAM,
	CONFIG_KEY_NET_MAC,
	CONFIG_KEY_NET_HEALTH_CHECK_INTERVAL,
	CONFIG_KEY_WS_PING_INTERVAL,
	CONFIG_KEY_SERVER_URL,
	CONFIG_KEY_SERVER_ID,
	CONFIG_KEY_SERVER_PASS,
	CONFIG_KEY_PLC_MAC,
	CONFIG_KEY_OCPP_CHECKPOINT,
	CONFIG_KEY_OCPP_CONFIG, /* ocpp configuration */
	CONFIG_KEY_MAX,
} config_key_t;

/**
 * @brief Initialize the configuration module.
 *
 * @param[in] nvs Pointer to the key-value store structure.
 *
 * @return 0 on success, negative error code on failure.
 */
int config_init(struct kvstore *nvs);

/**
 * @brief Read a configuration value.
 *
 * @param[in] key The configuration key to read.
 * @param[out] buf Buffer to store the read value.
 * @param[in] bufsize Size of the buffer.
 *
 * @return 0 on success, negative error code on failure.
 */
int config_read(config_key_t key, void *buf, size_t bufsize);

/**
 * @brief Save a configuration value.
 *
 * @param[in] key The configuration key to save.
 * @param[in] data Pointer to the data to save.
 * @param[in] datasize Size of the data to save.
 *
 * @return 0 on success, negative error code on failure.
 */
int config_save(config_key_t key, const void *data, size_t datasize);

/**
 * @brief Delete a configuration value.
 *
 * @param[in] key The configuration key to delete.
 *
 * @return 0 on success, negative error code on failure.
 */
int config_delete(config_key_t key);

/**
 * @brief Get the configuration key from a string.
 *
 * @param[in] keystr The string representation of the key.
 *
 * @return The configuration key.
 */
config_key_t config_get_key(const char *keystr);

/**
 * @brief Get the string representation of a configuration key.
 *
 * @param[in] key The configuration key.
 *
 * @return The string representation of the key.
 */
const char *config_get_keystr(config_key_t key);

/**
 * @brief Reset the configuration value for a specific key.
 *
 * This function resets the configuration value associated with the given key
 * to its default value. If the key does not exist, no action is performed.
 *
 * @param[in] key The configuration key for which to reset the value.
 *
 * @return 0 if the operation was successful, or a negative error code if the
 * operation failed.
 */
int config_reset(config_key_t key);

#if defined(__cplusplus)
}
#endif

#endif /* CONFIG_H */
