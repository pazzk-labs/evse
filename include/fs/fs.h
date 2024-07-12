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

#ifndef FS_H
#define FS_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stddef.h>
#include "libmcu/flash.h"

#define FS_FILENAME_MAX		32

typedef enum {
	FS_FILE_TYPE_FILE,
	FS_FILE_TYPE_DIR,
} fs_file_t;

struct fs;

typedef void (*fs_dir_cb_t)(struct fs *fs,
		const fs_file_t type, const char *name, void *ctx);

struct fs_api {
	int (*mount)(struct fs *self);
	int (*unmount)(struct fs *self);
	int (*read)(struct fs *self,
			const char *filepath, const size_t offset,
			void *buf, const size_t bufsize);
	int (*write)(struct fs *self,
			const char *filepath, const size_t offset,
			const void *data, const size_t datasize);
	int (*append)(struct fs *self, const char *filepath,
			const void *data, const size_t datasize);
	int (*erase)(struct fs *self, const char *filepath);
	int (*size)(struct fs *self, const char *filepath, size_t *size);
	int (*dir)(struct fs *self,
			const char *path, fs_dir_cb_t cb, void *cb_ctx);
	int (*usage)(struct fs *self, size_t *used, size_t *total);
};

static inline int fs_mount(struct fs *self) {
	return ((struct fs_api *)self)->mount(self);
}

static inline int fs_unmount(struct fs *self) {
	return ((struct fs_api *)self)->unmount(self);
}

static inline int fs_read(struct fs *self,
		const char *filepath, const size_t offset,
		void *buf, const size_t bufsize) {
	return ((struct fs_api *)self)->read(self,
			filepath, offset, buf, bufsize);
}

static inline int fs_write(struct fs *self,
		const char *filepath, const size_t offset,
		const void *data, const size_t datasize) {
	return ((struct fs_api *)self)->write(self,
			filepath, offset, data, datasize);
}

static inline int fs_append(struct fs *self, const char *filepath,
		const void *data, const size_t datasize) {
	return ((struct fs_api *)self)->append(self, filepath, data, datasize);
}

static inline int fs_delete(struct fs *self, const char *filepath) {
	return ((struct fs_api *)self)->erase(self, filepath);
}

static inline int fs_size(struct fs *self, const char *filepath, size_t *size) {
	return ((struct fs_api *)self)->size(self, filepath, size);
}

static inline int fs_dir(struct fs *self,
		const char *path, fs_dir_cb_t cb, void *cb_ctx) {
	return ((struct fs_api *)self)->dir(self, path, cb, cb_ctx);
}

static inline int fs_usage(struct fs *self, size_t *used, size_t *total) {
	return ((struct fs_api *)self)->usage(self, used, total);
}

struct fs *fs_create(struct flash *flash);
void fs_destroy(struct fs *fs);

#if defined(__cplusplus)
}
#endif

#endif /* FS_H */
