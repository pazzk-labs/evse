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

#define UID_ID_MAXLEN		21

/* Avoid conflict with POSIX's uid_t by using uid_id_t instead. */
typedef uint8_t uid_id_t[UID_ID_MAXLEN];

typedef enum {
	UID_STATUS_UNKNOWN,  /**< ID has not been processed yet. */
	UID_STATUS_ACCEPTED, /**< ID is valid and recognized, access allowed. */
	UID_STATUS_BLOCKED,  /**< ID is explicitly blocked from access. */
	UID_STATUS_EXPIRED,  /**< ID was valid but has expired. */
	UID_STATUS_INVALID,  /**< ID is invalid, blacklisted, or corrupted. */
	UID_STATUS_NO_ENTRY  /**< ID is not found in the local cache or store. */
} uid_status_t;

typedef void (*uid_update_cb_t)(const uid_id_t id, uid_status_t status,
		time_t expiry, void *ctx);

struct uid_store;
struct fs;

struct uid_store_config {
	struct fs *fs; /**< Filesystem backend to use. */
	const char *ns; /**< Filesystem namespace for isolation (e.g., "localList", "cache"). */
	uint16_t capacity; /**< Maximum number of entries in RAM. */
};

/**
 * @brief Creates and initializes a UID store instance.
 *
 * This initializes both in-memory cache and backing storage under the specified
 * namespace.
 *
 * @param[in] config  Store configuration including fs, namespace, and cache
 * size.
 *
 * @return Pointer to initialized store instance, or NULL on failure.
 */
struct uid_store *uid_store_create(const struct uid_store_config *config);

/**
 * @brief Destroys a previously created UID store instance.
 *
 * Frees all associated memory and internal structures.
 *
 * @param[in] store The UID store instance to destroy.
 */
void uid_store_destroy(struct uid_store *store);

/**
 * @brief Updates or inserts a UID entry into the store (cache + persistent
 * storage).
 *
 * This function is typically called when a response is received from the
 * server, or when a local decision is made to override the cached status.
 *
 * @param[in] store  The UID store instance.
 * @param[in] id     The UID to update.
 * @param[in] pid    Provisioning or parent ID to associate (optional).
 * @param[in] status The new status to associate with the UID.
 * @param[in] expiry The expiration time for the UID status.
 *
 * @return 0 on success, negative value on failure.
 */
int uid_update(struct uid_store *store, const uid_id_t id,
		const uid_id_t pid, uid_status_t status, time_t expiry);

/**
 * @brief Removes a UID entry from the store (RAM only).
 *
 * This function explicitly purges the UID from the in-memory cache.
 * It does not affect any persistent storage or remote server state.
 *
 * @param[in] store The UID store instance.
 * @param[in] id    The UID to remove from the cache.
 *
 * @return 0 on success, negative value if the UID was not found.
 */
int uid_delete(struct uid_store *store, const uid_id_t id);

/**
 * @brief Retrieves the status and expiry time of a UID.
 *
 * This function first checks the cache; if not found, it searches the
 * persistent storage. If the UID is found, its current status and expiry time
 * are returned.
 *
 * @param[in]  store  The UID store instance.
 * @param[in]  id     The UID to query.
 * @param[out] expiry Pointer to store the UID's expiry time (can be NULL).
 *
 * @return The status of the UID.
 */
uid_status_t uid_status(struct uid_store *store,
		const uid_id_t id, uid_id_t pid, time_t *expiry);

/**
 * @brief Clears all entries in the UID store.
 * 
 * This function removes all user identification (UID) entries from the
 * specified UID store, effectively resetting it to an empty state.
 * 
 * @param[in] store Pointer to the UID store structure to be cleared.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int uid_clear(struct uid_store *store);

/**
 * @brief Registers a callback to be invoked when a UID is updated.
 *
 * The callback is called when a UID entry is updated or inserted via
 * `uid_update()`, even if the new value is the same as the existing one. Only
 * one callback can be registered per store. Subsequent calls overwrite the
 * previous registration.
 *
 * @param[in] store The UID store instance.
 * @param[in] cb    The callback function to register.
 * @param[in] ctx   A user-defined context pointer passed to the callback.
 *
 * @return 0 on success, negative value on failure.
 */
int uid_register_update_cb(struct uid_store *store,
		uid_update_cb_t cb, void *ctx);

#if defined(__cplusplus)
}
#endif

#endif /* UID_H */
