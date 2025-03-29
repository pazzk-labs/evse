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

#ifndef SAFETY_H
#define SAFETY_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "safety_entry.h"

/**
 * @brief Error callback type for safety checks.
 *
 * Called when an entry returns a non-OK status during safety_check().
 *
 * @param[in] entry    The entry that failed the check.
 * @param[in] status   The status returned by the entry.
 * @param[in] ctx      User-defined context pointer.
 */
typedef void (*safety_error_callback_t)(struct safety_entry *entry,
		safety_entry_status_t status, void *ctx);

/**
 * @brief Iterator callback type for iterating all entries.
 *
 * Called once per registered entry in safety_iterate().
 *
 * @param[in] entry    The current entry.
 * @param[in] ctx      User-defined context pointer.
 */
typedef void (*safety_iterator_t)(struct safety_entry *entry, void *ctx);

struct safety;

/**
 * @brief Create a new safety instance.
 *
 * Allocates and initializes an empty safety context.
 *
 * @return Pointer to a new safety instance, or NULL on failure.
 */
struct safety *safety_create(void);

/**
 * @brief Destroy a safety instance and all of its entries.
 *
 * This function frees all resources associated with the given
 * safety context. All entries registered with the context will
 * be destroyed automatically by calling their destroy() method.
 *
 * @param[in] self Pointer to the safety instance to destroy.
 */
void safety_destroy(struct safety *self);

/**
 * @brief Add an entry to the safety context.
 *
 * Registers a new entry to be included in future safety checks.
 *
 * @param[in] self   Safety context.
 * @param[in] entry  Entry to be added.
 * @return 0 on success, or a negative error code on failure.
 *         -EINVAL: null pointer.  
 *         -EALREADY: entry already added.  
 *         -ENOMEM / -ENOSPC: capacity exceeded.
 */
int safety_add(struct safety *self, struct safety_entry *entry);

/**
 * @brief Add and enable an entry in one operation.
 *
 * Registers an entry to the safety context and immediately enables it.
 *
 * @param[in] self   Safety context.
 * @param[in] entry  Entry to add and enable.
 * @return 0 on success, or negative error code on failure.
 *         -EINVAL: null pointer.
 *         -EALREADY: already added.
 *         -ENOMEM / -ENOSPC: out of memory.
 *         -EIO: enable() failed after add, entry will be removed again.
 */
int safety_add_and_enable(struct safety *self, struct safety_entry *entry);

/**
 * @brief Remove an entry from the safety context.
 *
 * Deregisters the given entry, if present.
 *
 * @param[in] self   Safety context.
 * @param[in] entry  Entry to remove.
 * @return 0 on success, or a negative error code on failure.
 *         -EINVAL: null pointer.  
 *         -ENOENT: entry not found.
 */
int safety_remove(struct safety *self, struct safety_entry *entry);

/**
 * @brief Perform safety checks on all registered entries.
 *
 * Iterates through all registered entries and invokes their check()
 * method. For each entry that returns a non-OK status, the callback
 * is invoked.
 *
 * @param[in] self     Safety context.
 * @param[in] cb       Callback for reporting failed entries (nullable).
 * @param[in] cb_ctx   User-defined context pointer for callback.
 * @return 0 if all entries returned OK status (i.e. system is safe).  
 *         Otherwise, returns the number of failed entries (> 0).  
 *         Returns a negative error code on failure:
 *         -EINVAL: null pointer.
 */
int safety_check(struct safety *self, safety_error_callback_t cb, void *cb_ctx);

/**
 * @brief Iterate over all registered entries.
 *
 * Invokes the given callback once per registered entry.
 *
 * @param[in] self     Safety context.
 * @param[in] cb       Callback to invoke for each entry.
 * @param[in] cb_ctx   User-defined context pointer for callback.
 * @return 0 on success, or a negative error code on failure.
 *         -EINVAL: null pointer.
 */
int safety_iterate(struct safety *self, safety_iterator_t cb, void *cb_ctx);

#if defined(__cplusplus)
}
#endif

#endif /* SAFETY_H */
