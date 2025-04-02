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

#ifndef UID_H
#define UID_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <time.h>

#define UID_ID_MAXLEN		20

/* Avoid conflict with POSIX's uid_t by using uid_id_t instead. */
typedef uint8_t uid_id_t[UID_ID_MAXLEN];

typedef enum {
	UID_STATUS_UNKNOWN, /**< ID has not been processed yet. */
	UID_STATUS_ACCEPTED, /**< ID is valid and recognized, access allowed. */
	UID_STATUS_BLOCKED, /**< ID is explicitly blocked from access. */
	UID_STATUS_EXPIRED, /**< ID was valid but has expired. */
	UID_STATUS_INVALID, /**< ID is invalid, blacklisted, or corrupted. */
	UID_STATUS_NO_ENTRY /**< ID is not found in the local cache. */
} uid_status_t;

typedef void (*uid_update_cb_t)(const uid_id_t id, uid_status_t status,
		time_t expiry, void *ctx);

int uid_init(void);
void uid_deinit(void);

/**
 * @brief Updates the local UID cache with a new status and expiry time.
 *
 * This function is typically called when a response is received from the server,
 * or when a local decision is made to override the cached status.
 *
 * @param[in] id     The UID to update.
 * @param[in] status The new status to associate with the UID.
 * @param[in] expiry The expiration time for the UID status.
 * o
 * @return 0 on success, negative value on failure.
 */
int uid_update_cache(const uid_id_t id, uid_status_t status, time_t expiry);

/**
 * @brief Removes a UID entry from the local cache.
 *
 * This function explicitly purges the UID from the local memory cache.
 * It does not affect any persistent storage or remote server state.
 *
 * @param[in] id The UID to remove from the cache.
 *
 * @return 0 on success, negative value if the UID was not found.
 */
int uid_delete(const uid_id_t id);

/**
 * @brief Retrieves the status and expiry time of a UID from the local cache.
 *
 * If the UID is found, its current status and expiry time are returned.
 * If the UID is not found, UID_STATUS_NO_ENTRY is returned.
 *
 * @param[in]  id     The UID to query.
 * @param[out] expiry Pointer to store the UID's expiry time (can be NULL).
 *
 * @return The status of the UID.
 */
uid_status_t uid_status(const uid_id_t id, time_t *expiry);

/**
 * @brief Registers a callback to be invoked when a UID is updated.
 *
 * The callback is called when the cache is updated with new UID information,
 * typically after a server response or local status update.
 * Only one callback can be registered at a time. Subsequent calls overwrite
 * the previous registration.
 *
 * @param[in] cb   The callback function to register.
 * @param[in] ctx  A user-defined context pointer to pass to the callback.
 *
 * @return 0 on success, negative value on failure.
 */
int uid_register_update_cb(uid_update_cb_t cb, void *ctx);

#if defined(__cplusplus)
}
#endif

#endif /* UID_H */
