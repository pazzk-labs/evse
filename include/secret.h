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

#ifndef SECRET_H
#define SECRET_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "libmcu/kvstore.h"
#include <stddef.h>

typedef enum {
	SECRET_KEY_IMAGE_AES128_KEY,
	SECRET_KEY_X509_KEY,
	SECRET_KEY_X509_KEY_CSR,
	SECRET_KEY_MAX,
} secret_key_t;

/**
 * @brief Initialize the secret management module.
 *
 * @param[in] nvs Pointer to the key-value store structure.
 *
 * @return 0 on success, negative error code on failure.
 */
int secret_init(struct kvstore *nvs);

/**
 * @brief Read a secret value.
 *
 * @param[in] key The secret key to read.
 * @param[out] buf Buffer to store the read value.
 * @param[in] bufsize Size of the buffer.
 *
 * @return 0 on success, negative error code on failure.
 */
int secret_read(secret_key_t key, void *buf, size_t bufsize);

/**
 * @brief Save secret data associated with a given key.
 *
 * This function saves the secret data associated with the specified key.
 *
 * @param[in] key The key associated with the secret data.
 * @param[in] data Pointer to the data to be saved.
 * @param[in] datasize Size of the data to be saved.
 *
 * @return int 0 on success, or a negative error code on failure.
 */
int secret_save(secret_key_t key, const void *data, size_t datasize);

#if defined(__cplusplus)
}
#endif

#endif /* SECRET_H */
