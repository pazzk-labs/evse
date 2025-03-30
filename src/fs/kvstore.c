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

#include "fs/kvstore.h"
#include <stdlib.h>
#include "libmcu/compiler.h"

struct kvstore {
	struct kvstore_api api;
	struct fs *fs;
};

static int do_write(struct kvstore *self,
		char const *key, void const *value, size_t size)
{
	return fs_write(self->fs, key, 0, value, size);
}

static int do_read(struct kvstore *self,
			char const *key, void *buf, size_t size)
{
	return fs_read(self->fs, key, 0, buf, size);
}

static int do_erase(struct kvstore *self, char const *key)
{
	return fs_delete(self->fs, key);
}

static int do_open(struct kvstore *self, char const *ns)
{
	unused(self);
	unused(ns);
	return 0;
}

static void do_close(struct kvstore *self)
{
	unused(self);
}

struct kvstore *fs_kvstore_create(struct fs *fs)
{
	struct kvstore *fs_kvstore =
		(struct kvstore *)malloc(sizeof(*fs_kvstore));

	fs_kvstore->api = (struct kvstore_api) {
		.write = do_write,
		.read = do_read,
		.clear = do_erase,
		.open = do_open,
		.close = do_close,
	};

	fs_kvstore->fs = fs;

	return fs_kvstore;
}

void fs_kvstore_destroy(struct kvstore *kvstore)
{
	free(kvstore);
}
