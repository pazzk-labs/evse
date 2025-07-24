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

#include "libmcu/flash.h"
#include <errno.h>
#include <string.h>
#include "libmcu/compiler.h"

#define FAKE_STORAGE_SIZE	(8 * 1024 * 1024)

struct flash {
	struct flash_api api;
	uint8_t storage[FAKE_STORAGE_SIZE];
};

static int do_erase(struct flash *self, uintptr_t offset, size_t size)
{
	memset(&self->storage[offset], 0xff, size);
	return 0;
}

static int do_write(struct flash *self,
		uintptr_t offset, const void *data, size_t len)
{
	const uint8_t *src = (const uint8_t *)data;
	uint8_t *dst = &self->storage[offset];
	memcpy(dst, src, len);
	return 0;
}

static int do_read(struct flash *self, uintptr_t offset, void *buf, size_t len)
{
	const uint8_t *src = &self->storage[offset];
	memcpy(buf, src, len);
	return 0;
}

static size_t do_size(struct flash *self)
{
	unused(self);
	return 0;
}

struct flash *flash_create(int partition)
{
	static struct flash fs_partition = {
		.api = {
			.erase = do_erase,
			.write = do_write,
			.read = do_read,
			.size = do_size,
		},
	};

	if (partition != 0) {
		return NULL;
	}

	return &fs_partition;
}
