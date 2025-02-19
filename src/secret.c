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

#include "secret.h"

#include <string.h>
#include <errno.h>
#include <stdbool.h>

#define NAMESPACE		"secret"

struct secret {
	struct kvstore *nvs;
};

static struct secret secret;

static const char *secrets_tbl[] = {
	/* Do not exceed 15 characters. "123456789012345" */
	[SECRET_KEY_IMAGE_AES128_KEY] = "dfu/image.key",
	[SECRET_KEY_X509_KEY]         = "x509/dev.key",
	[SECRET_KEY_X509_KEY_CSR]     = "x509/dev.csr",
};

static const char *get_keystr_from_key(secret_key_t key)
{
	if (key >= SECRET_KEY_MAX) {
		return NULL;
	}

	return secrets_tbl[key];
}

int secret_read(secret_key_t key, void *buf, size_t bufsize)
{
	const char *keystr = get_keystr_from_key(key);

	if (!keystr) {
		return -ENOENT;
	}

	return kvstore_read(secret.nvs, keystr, buf, bufsize);
}

int secret_init(struct kvstore *nvs)
{
	memset(&secret, 0, sizeof(secret));

	int err = kvstore_open(nvs, NAMESPACE);
	secret.nvs = nvs;

	return err;
}
