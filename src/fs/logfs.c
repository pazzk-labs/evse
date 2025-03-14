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

#include "fs/logfs.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include "libmcu/list.h"
#include "libmcu/ringbuf.h"

#define LOGNAME_MAXLEN		14
#define BASE_PATH_MAXLEN	(FS_FILENAME_MAX - LOGNAME_MAXLEN - 1)

struct file {
	time_t timestamp;
	size_t size;
	struct list link;
};

struct cache {
	struct ringbuf *buffer;
	time_t timestamp;
};

struct logfs {
	struct fs *fs;
	const char *base_path;
	size_t max_size;
	size_t max_logs;

	struct cache cache;

	struct list log_list;
};

static void get_filepath(const time_t ts, const char *basedir,
		char *buf, const size_t bufsize)
{
	snprintf(buf, bufsize-1, "%s/%"PRId64, basedir, ts);
}

static void add_list_sorted(struct file *file, struct list *head)
{
	struct list *p;
	struct list *t = head;
	list_for_each(p, head) {
		struct file *entry = list_entry(p, struct file, link);
		if (file->timestamp == entry->timestamp) {
			/* replace the existing one */
			entry->size = file->size;
			free(file);
			return;
		} else if (file->timestamp < entry->timestamp) {
			list_add(&file->link, t);
			return;
		}
		t = p;
	}
	list_add_tail(&file->link, head);
}

static void add_list(struct logfs *self, const time_t ts, const size_t logsize)
{
	struct file *file = (struct file *)malloc(sizeof(struct file));
	if (!file) {
		return;
	}

	file->timestamp = ts;
	file->size = logsize;
	add_list_sorted(file, &self->log_list);
}

static int add_file(struct fs *fs, const char *basedir,
		const time_t ts, const void *data, const size_t datasize)
{
	char filepath[FS_FILENAME_MAX+1];
	get_filepath(ts, basedir, filepath, sizeof(filepath));
	return fs_append(fs, filepath, data, datasize);
}

static int delete_file(struct fs *fs, const char *basedir, const time_t ts)
{
	char filepath[FS_FILENAME_MAX+1];
	get_filepath(ts, basedir, filepath, sizeof(filepath));
	return fs_delete(fs, filepath);
}

static bool delete_list(struct list *head, const time_t ts)
{
	struct list *p;
	list_for_each(p, head) {
		struct file *file = list_entry(p, struct file, link);
		if (file->timestamp == ts) {
			list_del(p, head);
			free(file);
			return true;
		}
	}
	return false;
}

static int delete_log(struct logfs *self, const time_t ts)
{
	int err;

	if ((err = delete_file(self->fs, self->base_path, ts)) == 0) {
		err = delete_list(&self->log_list, ts)? 0 : -ENOENT;
	}

	return err;
}

static struct file *find_log(struct logfs *self, const time_t ts)
{
	struct list *p;
	list_for_each(p, &self->log_list) {
		struct file *file = list_entry(p, struct file, link);
		if (file->timestamp == ts) {
			return file;
		}
	}

	return NULL;
}

static size_t compute_total_size(struct logfs *self)
{
	size_t total = 0;

	struct list *p;
	list_for_each(p, &self->log_list) {
		struct file *file = list_entry(p, struct file, link);
		total += file->size;
	}

	return total;
}

static void on_dir_scan(struct fs *fs,
		const fs_file_t type, const char *name, void *ctx)
{
	(void)type;

	struct logfs *self = (struct logfs *)ctx;
	char filepath[FS_FILENAME_MAX+1];

	snprintf(filepath, sizeof(filepath)-1, "%s/%s", self->base_path, name);

	time_t timestamp = strtoll(name, NULL, 10);
	size_t logsize;
	fs_size(fs, filepath, &logsize);
	add_list(self, timestamp, logsize);
}

static void reclaim_by_count(struct logfs *self, const size_t n)
{
	const size_t count = (size_t)list_count(&self->log_list);

	if (!self->max_logs || (count + n) <= self->max_logs) {
		return;
	}

	for (size_t i = 0; i < (count + n) - self->max_logs; i++) {
		struct list *p = list_first(&self->log_list);
		struct file *file = list_entry(p, struct file, link);
		delete_log(self, file->timestamp);
	}
}

static void reclaim_by_size(struct logfs *self, const size_t bytes_to_append)
{
	struct list *p;
	struct list *n;

	if (!self->max_size || ((compute_total_size(self) + bytes_to_append)
			<= self->max_size)) {
		return;
	}

	list_for_each_safe(p, n, &self->log_list) {
		struct file *file = list_entry(p, struct file, link);
		delete_log(self, file->timestamp);

		if ((compute_total_size(self) + bytes_to_append)
				<= self->max_size) {
			break;
		}
	}
}

static void reclaim(struct logfs *self, const size_t bytes_to_append)
{
	if (list_empty(&self->log_list)) {
		fs_dir(self->fs, self->base_path, on_dir_scan, self);
	}

	reclaim_by_count(self, bytes_to_append? 1 : 0);
	reclaim_by_size(self, bytes_to_append);
}

static void flush_logcache(struct logfs *self,
		const time_t ts, const size_t bytes_to_append)
{
	reclaim(self, bytes_to_append);

	while (ringbuf_length(self->cache.buffer) > 0) {
		int err;
		size_t len;
		const void *p;

		if ((p = ringbuf_peek_pointer(self->cache.buffer,
				0, &len)) == NULL || len == 0) {
			return; /* will retry on next flush */
		}
		if ((err = add_file(self->fs, self->base_path, ts, p, len))
				< 0) {
			return;
		}

		ringbuf_consume(self->cache.buffer, (size_t)err);
	}

	char filepath[FS_FILENAME_MAX+1];
	size_t logsize;
	get_filepath(ts, self->base_path, filepath, sizeof(filepath));
	if (fs_size(self->fs, filepath, &logsize) >= 0 && logsize > 0) {
		add_list(self, ts, logsize);
	}
}

int logfs_write(struct logfs *self,
		const time_t timestamp, const void *log, const size_t logsize)
{
	const uint8_t *p = (const uint8_t *)log;

	if (logsize > self->max_size) {
		return -EFBIG;
	}

	if (timestamp != self->cache.timestamp) {
		flush_logcache(self, self->cache.timestamp,
				ringbuf_length(self->cache.buffer) + logsize);
		self->cache.timestamp = timestamp;
	}

	size_t written = ringbuf_write(self->cache.buffer, p, logsize);
	size_t written_total = written;

	while (written && written_total < logsize) {
		const size_t remain = logsize - written_total;
		flush_logcache(self, timestamp,
				ringbuf_capacity(self->cache.buffer) + remain);
		written = ringbuf_write(self->cache.buffer,
				&p[written], remain);
		written_total += written;
	}

	return (int)written_total;
}

int logfs_read(struct logfs *self, const time_t timestamp,
		const size_t offset, void *buf, const size_t bufsize)
{
	char filepath[FS_FILENAME_MAX+1];
	get_filepath(timestamp, self->base_path, filepath, sizeof(filepath));
	return fs_read(self->fs, filepath, offset, buf, bufsize);
}

size_t logfs_size(struct logfs *self, const time_t timestamp)
{
	struct file *file;

	if (!timestamp) {
		return compute_total_size(self);
	}

	if ((file = find_log(self, timestamp)) == NULL) {
		return 0;
	}

	return file->size;
}

size_t logfs_count(struct logfs *self)
{
	size_t count = 0;

	struct list *p;
	list_for_each(p, &self->log_list) {
		count++;
	}
	
	return count;
}

int logfs_dir(struct logfs *self, logfs_dir_cb_t cb, void *cb_ctx,
		const size_t max_files)
{
	struct list *p;
	struct list *n;
	size_t count = 0;

	list_for_each_safe(p, n, &self->log_list) {
		struct file *file = list_entry(p, struct file, link);
		(*cb)(self, file->timestamp, cb_ctx);
		if (max_files && ++count >= max_files) {
			break;
		}
	}

	return 0;
}

int logfs_delete(struct logfs *self, const time_t timestamp)
{
	return delete_log(self, timestamp);
}

int logfs_clear(struct logfs *self)
{
	char filepath[FS_FILENAME_MAX+1];
	int err = 0;

	struct list *p;
	struct list *n;
	list_for_each_safe(p, n, &self->log_list) {
		struct file *file = list_entry(p, struct file, link);

		get_filepath(file->timestamp, self->base_path,
				filepath, sizeof(filepath));
		err |= fs_delete(self->fs, filepath);
		list_del(p, &self->log_list);

		free(file);
	}

	return err;
}

int logfs_flush(struct logfs *self)
{
	flush_logcache(self, self->cache.timestamp,
			ringbuf_length(self->cache.buffer));
	return 0;
}

struct logfs *logfs_create(struct fs *fs, const char *base_path,
		const size_t max_size, const size_t max_logs, size_t cache_size)
{
	if (!fs || !base_path || strlen(base_path) > BASE_PATH_MAXLEN) {
		return NULL;
	}

	struct logfs *logfs = (struct logfs *)malloc(sizeof(struct logfs));

	if (logfs) {
		list_init(&logfs->log_list);
		logfs->fs = fs;
		logfs->base_path = base_path;
		logfs->max_size = max_size;
		logfs->max_logs = max_logs;

		if (cache_size < LOGFS_MIN_CACHE_SIZE) {
			cache_size = LOGFS_MIN_CACHE_SIZE;
		}

		memset(&logfs->cache, 0, sizeof(logfs->cache));
		if ((logfs->cache.buffer = ringbuf_create(cache_size))
				== NULL) {
			free(logfs);
			return NULL;
		}
	}

	return logfs;
}

void logfs_destroy(struct logfs *self)
{
	if (!self) {
		return;
	}

	struct list *p;
	struct list *n;
	list_for_each_safe(p, n, &self->log_list) {
		struct file *file = list_entry(p, struct file, link);
		list_del(p, &self->log_list);
		free(file);
	}

	ringbuf_destroy(self->cache.buffer);

	free(self);
}
