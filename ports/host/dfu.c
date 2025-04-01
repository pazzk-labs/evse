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

#include "libmcu/compiler.h"

struct dfu {
	struct dfu_image_header header;
};

bool dfu_is_valid_header(const struct dfu_image_header *header)
{
	return header->magic == 0xC0DEu && header->type == DFU_TYPE_APP;
}

dfu_error_t dfu_write(struct dfu *dfu, uint32_t offset,
		const void *data, size_t datasize)
{
	unused(dfu);
	unused(offset);
	unused(data);
	unused(datasize);
	return DFU_ERROR_NONE;
}

dfu_error_t dfu_prepare(struct dfu *dfu, const struct dfu_image_header *header)
{
	unused(dfu);
	unused(header);
	return DFU_ERROR_NONE;
}

dfu_error_t dfu_abort(struct dfu *dfu)
{
	unused(dfu);
	return DFU_ERROR_NONE;
}

dfu_error_t dfu_finish(struct dfu *dfu)
{
	unused(dfu);
	return DFU_ERROR_NONE;
}

dfu_error_t dfu_commit(struct dfu *dfu)
{
	unused(dfu);
	return DFU_ERROR_NONE;
}

struct dfu *dfu_new(size_t data_block_size)
{
	unused(data_block_size);
	static struct dfu dfu;
	return &dfu;
}

void dfu_delete(struct dfu *dfu)
{
	unused(dfu);
}
