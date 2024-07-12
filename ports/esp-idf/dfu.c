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

#include "libmcu/dfu.h"

#include <stdlib.h>
#include <string.h>

#include "libmcu/metrics.h"
#include "esp_ota_ops.h"

struct dfu {
	struct dfu_image_header header;

	esp_ota_handle_t ota_handle;
	const esp_partition_t *slot;
};

bool dfu_is_valid_header(const struct dfu_image_header *header)
{
	return header->magic == 0xC0DEu && header->type == DFU_TYPE_APP;
}

dfu_error_t dfu_write(struct dfu *dfu, uint32_t offset,
		const void *data, size_t datasize)
{
	if (esp_ota_write(dfu->ota_handle, data, datasize) != ESP_OK) {
		metrics_increase(DFUIOErrorCount);
		return DFU_ERROR_IO;
	}

	return datasize;
}

dfu_error_t dfu_prepare(struct dfu *dfu, const struct dfu_image_header *header)
{
	memcpy(&dfu->header, header, sizeof(*header));

	if (esp_ota_begin(dfu->slot, dfu->header.datasize, &dfu->ota_handle)
			!= ESP_OK) {
		metrics_increase(DFUPrepareErrorCount);
		return DFU_ERROR_INVALID_SLOT;
	}

	return DFU_ERROR_NONE;
}

dfu_error_t dfu_abort(struct dfu *dfu)
{
	esp_ota_abort(dfu->ota_handle);
	return DFU_ERROR_NONE;
}

dfu_error_t dfu_finish(struct dfu *dfu)
{
	if (esp_ota_end(dfu->ota_handle) != ESP_OK) {
		metrics_increase(DFUFinishErrorCount);
		return DFU_ERROR_INVALID_IMAGE;
	}

	return DFU_ERROR_NONE;
}

dfu_error_t dfu_commit(struct dfu *dfu)
{
	if (esp_ota_set_boot_partition(dfu->slot) != ESP_OK) {
		metrics_increase(DFUCommitErrorCount);
		return DFU_ERROR_SLOT_UPDATE_FAIL;
	}

	metrics_increase(DFUSuccessCount);
	return DFU_ERROR_NONE;
}

struct dfu *dfu_new(size_t data_block_size)
{
	metrics_increase(DFURequestCount);
	struct dfu *p = (struct dfu *)calloc(1, sizeof(struct dfu));

	if (p) {
		p->slot = esp_ota_get_next_update_partition(NULL);
	}

	return p;
}

void dfu_delete(struct dfu *dfu)
{
	free(dfu);
}
