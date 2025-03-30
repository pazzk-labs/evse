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

#include "fs/fs.h"
#include <errno.h>
#include <pthread.h>
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include "lfs.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
#include "logger.h"
#include "libmcu/metrics.h"
#include "libmcu/board.h"

#define LFS_FLASH_SIZE			(8 * 1024/*MiB*/ * 1024/*KiB*/)
#define LFS_FLASH_SECTOR_SIZE		(4 * 1024/*KiB*/)
#define LFS_FLASH_PAGE_SIZE		256

#if !defined(MIN)
#define MIN(a, b)			(((a) > (b))? (b) : (a))
#endif

struct fs {
	struct fs_api api;
	struct flash *flash;
	lfs_t lfs;
#if defined(LFS_THREADSAFE)
	pthread_mutex_t mutex;
#endif
};

static struct fs fs;

#if defined(LFS_THREADSAFE)
static int lock_fs(const struct lfs_config *cfg)
{
	(void)cfg;
	return pthread_mutex_lock(&fs.mutex);
}

static int unlock_fs(const struct lfs_config *cfg)
{
	(void)cfg;
	return pthread_mutex_unlock(&fs.mutex);
}
#endif

static int read_block(const struct lfs_config *c, lfs_block_t block,
		lfs_off_t off, void *buffer, lfs_size_t size)
{
	return flash_read(fs.flash, block * c->block_size + off, buffer, size);
}

static int write_block(const struct lfs_config *c, lfs_block_t block,
		lfs_off_t off, const void *buffer, lfs_size_t size)
{
	return flash_write(fs.flash, block * c->block_size + off, buffer, size);
}

static int erase_block(const struct lfs_config *c, lfs_block_t block)
{
	return flash_erase(fs.flash, block * c->block_size, c->block_size);
}

static int sync_fs(const struct lfs_config *c)
{
	unused(c);
	return 0;
}

static int find_dir_trailling_slash(const char *path)
{
	for (int i = 0; path && path[i] != '\0'; i++) {
		if (path[i] == '/') {
			return i;
		}
	}
	return 0;
}

static const char *skip_leading_slashes(const char *path)
{
	while (path && *path == '/') {
		path++;
	}
	return path;
}

static int create_directory(lfs_t *fs_handle, const char *filepath)
{
	const char *p = skip_leading_slashes(filepath);
	int offset = 0;

	for (int i = 0; p[i] != '\0'; i++) {
		int slash = find_dir_trailling_slash(&p[i]);
		if (!slash) {
			continue;
		}

		offset += slash;

		char dir[FS_FILENAME_MAX+1] = { 0, };
		memcpy(dir, p, MIN((size_t)offset, sizeof(dir)-1));
		int err = lfs_mkdir(fs_handle, dir);

		if (err != 0 && err != LFS_ERR_EXIST) {
			return err;
		}

		i += slash;
		offset += 1; /* slash */
	}

	return 0;
}

static int write_core(struct fs *self,
		const char *filepath, const size_t offset,
		const void *data, const size_t datasize, const int flags)
{
	lfs_file_t file;

	int err = create_directory(&self->lfs, filepath);
	if (err != 0) {
		goto out;
	}

	if ((err = lfs_file_open(&self->lfs, &file, filepath, flags))
			!= LFS_ERR_OK) {
		err = err == LFS_ERR_NOENT? -ENOENT : err;
		goto out;
	}

	if (offset) {
		if ((err = lfs_file_seek(&self->lfs, &file,
				(lfs_soff_t)offset, LFS_SEEK_SET)) < 0) {
			goto out;
		}
	}

	err = lfs_file_write(&self->lfs, &file, data, (lfs_size_t)datasize);
	err |= lfs_file_close(&self->lfs, &file);

out:
	metrics_increase(FileSystemWriteCount);
	return err;
}

static int do_write(struct fs *self, const char *filepath, const size_t offset,
		const void *data, const size_t datasize)
{
	const uint32_t t0 = board_get_time_since_boot_ms();
	int err = write_core(self, filepath, offset, data, datasize,
			LFS_O_RDWR | LFS_O_CREAT);
	metrics_set_if_max(FileSystemWriteTimeMax,
			METRICS_VALUE(board_get_time_since_boot_ms() - t0));
	return err;
}

static int do_append(struct fs *self,
		const char *filepath, const void *data, const size_t datasize)
{
	const uint32_t t0 = board_get_time_since_boot_ms();
	int err = write_core(self, filepath, 0, data, datasize,
			LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND);
	metrics_set_if_max(FileSystemWriteTimeMax,
			METRICS_VALUE(board_get_time_since_boot_ms() - t0));
	return err;
}

static int do_read(struct fs *self, const char *filepath, const size_t offset,
		void *buf, const size_t bufsize)
{
	const uint32_t t0 = board_get_time_since_boot_ms();
	lfs_file_t file;

	int err = lfs_file_open(&self->lfs, &file, filepath, LFS_O_RDONLY);
	if (err != LFS_ERR_OK) {
		err = err == LFS_ERR_NOENT? -ENOENT : err;
		goto out;
	}

	const lfs_soff_t file_size = lfs_file_size(&self->lfs, &file);
	const lfs_size_t len = MIN((lfs_size_t)file_size, (lfs_size_t)bufsize);

	if (len <= 0) {
		err = -EBADF;
		goto out;
	}

	if (offset) {
		if ((err = lfs_file_seek(&self->lfs, &file,
				(lfs_soff_t)offset, LFS_SEEK_SET)) < 0) {
			goto out;
		}
	}

	err = lfs_file_read(&self->lfs, &file, buf, len);
	lfs_file_close(&self->lfs, &file);

	metrics_set_if_max(FileSystemReadTimeMax,
			METRICS_VALUE(board_get_time_since_boot_ms() - t0));
out:
	metrics_increase(FileSystemReadCount);
	return err;
}

static int do_delete(struct fs *self, const char *filepath)
{
	const uint32_t t0 = board_get_time_since_boot_ms();
	int err = lfs_remove(&self->lfs, filepath);
	metrics_set_if_max(FileSystemEraseTimeMax,
			METRICS_VALUE(board_get_time_since_boot_ms() - t0));
	metrics_increase(FileSystemEraseCount);
	return err == LFS_ERR_NOENT? -ENOENT : err;
}

static int do_size(struct fs *self, const char *filepath, size_t *size)
{
	lfs_file_t file;
	int err;

	if ((err = lfs_file_open(&self->lfs, &file, filepath, LFS_O_RDONLY))
			!= LFS_ERR_OK) {
		if (err == LFS_ERR_NOENT) {
			return -ENOENT;
		} else if (err == LFS_ERR_ISDIR) {
			return -EISDIR;
		}
		return err;
	}

	const lfs_soff_t file_size = lfs_file_size(&self->lfs, &file);
	*size = (size_t)file_size;

	return lfs_file_close(&self->lfs, &file);
}

static int do_dir(struct fs *self,
		const char *path, fs_dir_cb_t cb, void *cb_ctx)
{
	struct lfs_info info;
	lfs_dir_t dir;
	int err;
	int file_count = 0;
	const char *p = skip_leading_slashes(path);

	if ((err = lfs_dir_open(&self->lfs, &dir, p))) {
		return err == LFS_ERR_NOENT? -ENOENT : err;
	}

	while ((err = lfs_dir_read(&self->lfs, &dir, &info)) > 0) {
		if (strcmp(info.name, ".") == 0 ||
				strcmp(info.name, "..") == 0) {
			continue;
		}

		if (cb) {
			const fs_file_t type = info.type == LFS_TYPE_DIR?
				FS_FILE_TYPE_DIR : FS_FILE_TYPE_FILE;
			(*cb)(self, type, info.name, cb_ctx);
		}
		file_count++;
	}

	lfs_dir_close(&self->lfs, &dir);
	return err >= 0? file_count : err;
}

static int do_usage(struct fs *self, size_t *used, size_t *total)
{
	const lfs_ssize_t used_blocks = lfs_fs_size(&self->lfs);

	if (used_blocks < 0) {
		return used_blocks == LFS_ERR_NOENT? -ENOENT : used_blocks;
	}

	const lfs_ssize_t block_size = LFS_FLASH_SECTOR_SIZE;
	const lfs_ssize_t total_blocks = LFS_FLASH_SIZE / block_size;
	const size_t total_size = (size_t)(total_blocks * block_size);
	const size_t used_size = (size_t)(used_blocks * block_size);

	if (used) {
		*used = used_size;
	}
	if (total) {
		*total = total_size;
	}

	return 0;
}

static int do_mount(struct fs *self)
{
	/* keep it static as lfs doesn't hard-copy the cfg,
	 * but dereferences it. */
	static const struct lfs_config cfg = {
		.read = read_block,
		.prog = write_block,
		.erase = erase_block,
		.sync = sync_fs,
#if defined(LFS_THREADSAFE)
		.lock = lock_fs,
		.unlock = unlock_fs,
#endif

		.read_size = LFS_FLASH_PAGE_SIZE,
		.prog_size = LFS_FLASH_PAGE_SIZE,
		.block_size = LFS_FLASH_SECTOR_SIZE,
		.block_count = LFS_FLASH_SIZE / LFS_FLASH_SECTOR_SIZE,
		.cache_size = LFS_FLASH_PAGE_SIZE * 2,
		.lookahead_size = LFS_FLASH_PAGE_SIZE,
		.block_cycles = 500,
	};

	int err = lfs_mount(&self->lfs, &cfg);

	if (err) { /* This should only happen on the first boot. */
		info("Formatting LFS...");
		err = lfs_format(&self->lfs, &cfg);
		err |= lfs_mount(&self->lfs, &cfg);
	}

	if (err) {
		lfs_unmount(&self->lfs);
		error("Failed to mount LFS: %d", err);
		return -EIO;
	}

	return 0;
}

static int do_unmount(struct fs *self)
{
	return lfs_unmount(&self->lfs);
}

struct fs *fs_create(struct flash *flash)
{
	memset(&fs, 0, sizeof(fs));

	fs.flash = flash;
	fs.api = (struct fs_api) {
		.mount = do_mount,
		.unmount = do_unmount,
		.read = do_read,
		.write = do_write,
		.append = do_append,
		.erase = do_delete,
		.size = do_size,
		.dir = do_dir,
		.usage = do_usage,
	};

#if defined(LFS_THREADSAFE)
	pthread_mutex_init(&fs.mutex, NULL);
#endif

	return &fs;
}
