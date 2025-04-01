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
#include "esp_partition.h"
#include "libmcu/compiler.h"

#define FS_PARTITION			"fs"

struct flash {
	struct flash_api api;
};

static const esp_partition_t *get_partition(void)
{
	return esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
			ESP_PARTITION_SUBTYPE_DATA_LITTLEFS, FS_PARTITION);
}

static int do_erase(struct flash *self, uintptr_t offset, size_t size)
{
	const esp_partition_t *partition = get_partition();

	if (partition == NULL) {
		return -ENODEV;
	}

	esp_err_t err = esp_partition_erase_range(partition, offset, size);

	return err == ESP_OK ? 0 : -err;
}

static int do_write(struct flash *self,
		uintptr_t offset, const void *data, size_t len)
{
	const esp_partition_t *partition = get_partition();

	if (partition == NULL) {
		return -ENODEV;
	}

	esp_err_t err = esp_partition_write(partition, offset, data, len);

	return err == ESP_OK ? 0 : -err;
}

static int do_read(struct flash *self, uintptr_t offset, void *buf, size_t len)
{
	const esp_partition_t *partition = get_partition();

	if (partition == NULL) {
		return -ENODEV;
	}

	esp_err_t err = esp_partition_read(partition, offset, buf, len);

	return err == ESP_OK ? 0 : -err;
}

static size_t do_size(struct flash *self)
{
	unused(self);
	return -ENOTSUP;
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
