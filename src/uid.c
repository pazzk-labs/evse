/*
 * This file is part of the Pazzk project <https://pazzk.net/>.
 * Copyright (c) 2025 Pazzk <team@pazzk.net>.
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

#include "uid.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "libmcu/compiler.h"
#include "libmcu/hexdump.h"
#include "libmcu/metrics.h"
#include "libmcu/board.h"
#include "fs/fs.h"
#include "logger.h"

#define STORAGE_ROOT		"uid"
#define FILENAME_MAXLEN		16
#define FILEPATH_MAXLEN		16

struct uid {
	uid_id_t id;
	uid_id_t pid;
	time_t expiry;
	uid_status_t status;
} LIBMCU_PACKED;
static_assert(sizeof(struct uid) == 54,
		"Size of struct uid is not equal to 52 bytes");

struct uid_record {
	struct uid uid;
};

struct cache_entry {
	struct uid_record record;
};

struct uid_store {
	struct fs *fs;
	const char *ns;
	uid_update_cb_t cb;
	void *cb_ctx;

	struct cache_entry **cache;
	uint16_t capacity;

	char dir[FILEPATH_MAXLEN];
};

static uint32_t hash_uid(const uid_id_t id, uint16_t cap)
{
	uint32_t h = 2166136261u;

	for (size_t i = 0; i < sizeof(uid_id_t); i++) {
		h = (h ^ id[i]) * 16777619u;
	}

	return h % (uint32_t)cap;
}

static struct cache_entry *new_entry(void)
{
	struct cache_entry *entry =
		(struct cache_entry *)calloc(1, sizeof(*entry));

	if (!entry) {
		error("Failed to allocate memory for UID cache.");
	}

	debug("New cache entry allocated at %p", entry);

	return entry;
}

static void free_entry(struct cache_entry *entry)
{
	debug("Freeing cache entry at %p", entry);
	free(entry);
}

static void get_filepath(char *filepath, size_t maxlen,
		const char *ns, const char *dir, const char *filename)
{
	size_t len = 0;
	int t = snprintf(filepath, maxlen, "%s/%s", STORAGE_ROOT, ns);

	if (t > 0 && dir && strlen(dir)) {
		len += (size_t)t;
		t = snprintf(filepath + len, maxlen - len, "/%s", dir);
	}

	if (t > 0 && filename && strlen(filename)) {
		len += (size_t)t;
		t = snprintf(filepath + len, maxlen - len, "/%s", filename);
	}
}

static void get_filename(char *filepath, size_t maxlen,
		const char *ns, const uid_id_t id)
{
	snprintf(filepath, maxlen, "%s/%s/%02X/%02X.bin",
			STORAGE_ROOT, ns, id[0], id[1]);
}

static int read_entry_from_file(struct uid_store *store,
		const uid_id_t id, struct uid_record *record, size_t *offset)
{
	char filepath[FILEPATH_MAXLEN + FILENAME_MAXLEN];
	size_t filesize;

	get_filename(filepath, sizeof(filepath), store->ns, id);

	if (fs_size(store->fs, filepath, &filesize) < 0) {
		return -ENOENT;
	}

	for (size_t i = 0; i < filesize / sizeof(*record); i++) {
		if (fs_read(store->fs, filepath, i * sizeof(*record),
				record, sizeof(*record)) == sizeof(*record)) {
			if (memcmp(record->uid.id, id, sizeof(uid_id_t)) == 0) {
				if (offset) {
					*offset = i * sizeof(*record);
				}
				return 0;
			}
		}
	}

	return -ENOENT;
}

static int save_entry_into_file(struct uid_store *store,
		const struct uid_record *record)
{
	struct uid_record dummy;
	char filepath[FILEPATH_MAXLEN + FILENAME_MAXLEN];
	size_t offset;

	get_filename(filepath, sizeof(filepath), store->ns, record->uid.id);

	debug("Saving UID to file: %s", filepath);

	if (read_entry_from_file(store, record->uid.id, &dummy, &offset) == 0) {
		return fs_write(store->fs,
				filepath, offset, record, sizeof(*record));
	}

	return fs_append(store->fs, filepath, record, sizeof(*record));
}

/* FIXME: this will introduce internal fragmentation in flash */
static int remove_entry_from_file(struct uid_store *store, const uid_id_t id)
{
	struct uid_record dummy;
	char filepath[FILEPATH_MAXLEN + FILENAME_MAXLEN];
	size_t offset;

	get_filename(filepath, sizeof(filepath), store->ns, id);

	if (read_entry_from_file(store, id, &dummy, &offset) == 0) {
		memset(&dummy, 0, sizeof(dummy));
		return fs_write(store->fs,
				filepath, offset, &dummy, sizeof(dummy));
	}

	return -ENOENT;
}

static void on_clear_dir(struct fs *fs, const fs_file_t type,
		const char *filename, void *ctx)
{
	char filepath[FILEPATH_MAXLEN + FILENAME_MAXLEN];
	struct uid_store *store = (struct uid_store *)ctx;

	if (type == FS_FILE_TYPE_DIR) {
		const size_t len = strlen(store->dir);
		strncat(store->dir, filename, sizeof(store->dir) - 1);
		get_filepath(filepath, sizeof(filepath),
				store->ns, store->dir, NULL);
		info("recursive delete: %s", filepath);
		fs_dir(store->fs, filepath, on_clear_dir, store);
		store->dir[len] = '\0';
	}

	get_filepath(filepath, sizeof(filepath),
			store->ns, store->dir, filename);
	debug("deleting file: %s", filepath);

	if (fs_delete(fs, filepath) < 0) {
		error("Failed to delete file: %s", filepath);
	}
}

uid_status_t uid_status(struct uid_store *store,
		const uid_id_t id, uid_id_t pid, time_t *expiry)
{
	if (!store || !id) {
		return UID_STATUS_NO_ENTRY;
	}

	const uint32_t t0 = board_get_time_since_boot_ms();
	const uint32_t h = hash_uid(id, store->capacity);
	struct cache_entry *entry = store->cache[h];

	if (entry && memcmp(entry->record.uid.id, id, sizeof(uid_id_t)) == 0) {
		if (expiry) {
			*expiry = entry->record.uid.expiry;
		}
		if (pid) {
			memcpy(pid, entry->record.uid.pid, sizeof(uid_id_t));
		}
		return entry->record.uid.status;
	}

	if (!entry) {
		if (!(entry = new_entry())) {
			return UID_STATUS_NO_ENTRY;
		}
	}

	if (read_entry_from_file(store, id, &entry->record, NULL) == 0) {
		const uint32_t elapsed = board_get_time_since_boot_ms() - t0;
		metrics_set_if_max(UIDStatusTimeMax, METRICS_VALUE(elapsed));
		metrics_set_if_min(UIDStatusTimeMin, METRICS_VALUE(elapsed));

		char buf[sizeof(uid_id_t)*2+1];
		hexdump(buf, sizeof(buf), id, sizeof(uid_id_t));
		info("cache miss: %s", buf);

		store->cache[h] = entry;

		if (expiry) {
			*expiry = entry->record.uid.expiry;
		}
		if (pid) {
			memcpy(pid, entry->record.uid.pid, sizeof(uid_id_t));
		}

		return entry->record.uid.status;
	}

	free_entry(entry);

	return UID_STATUS_NO_ENTRY;
}

int uid_update(struct uid_store *store, const uid_id_t id,
		const uid_id_t pid, uid_status_t status, time_t expiry)
{
	if (!store || !id) {
		return -EINVAL;
	}

	const uint32_t t0 = board_get_time_since_boot_ms();
	const uint32_t h = hash_uid(id, store->capacity);
	struct cache_entry *entry = store->cache[h];

	if (!entry) {
		if ((entry = new_entry()) == NULL) {
			return -ENOMEM;
		}
		store->cache[h] = entry;
	}

	memcpy(entry->record.uid.id, id, sizeof(uid_id_t));
	if (pid) {
		memcpy(entry->record.uid.pid, pid, sizeof(uid_id_t));
	}
	entry->record.uid.status = status;
	entry->record.uid.expiry = expiry;

	if (save_entry_into_file(store, &entry->record) < 0) {
		error("Failed to save UID to flash.");
		return -EIO;
	}

	const uint32_t elapsed = board_get_time_since_boot_ms() - t0;
	metrics_set_if_max(UIDUpdateTimeMax, METRICS_VALUE(elapsed));
	metrics_set_if_min(UIDUpdateTimeMin, METRICS_VALUE(elapsed));

	char buf[sizeof(uid_id_t)*2+1];
	hexdump(buf, sizeof(buf), id, sizeof(uid_id_t));
	info("UID updated: %s", buf);

	if (store->cb) {
		(*store->cb)(id, status, expiry, store->cb_ctx);
	}

	return 0;
}

int uid_delete(struct uid_store *store, const uid_id_t id)
{
	if (!store || !id) {
		return -EINVAL;
	}

	const uint32_t h = hash_uid(id, store->capacity);
	struct cache_entry *entry = store->cache[h];

	if (entry && memcmp(entry->record.uid.id, id, sizeof(uid_id_t)) == 0) {
		free_entry(entry);
		store->cache[h] = NULL;
	}

	int err = remove_entry_from_file(store, id);

	if (err >= 0) {
		char buf[sizeof(uid_id_t)*2+1];
		hexdump(buf, sizeof(buf), id, sizeof(uid_id_t));
		info("UID deleted: %s", buf);
	}

	return err >= 0? 0 : err;
}

int uid_clear(struct uid_store *store)
{
	if (!store) {
		return -EINVAL;
	}

	const uint32_t t0 = board_get_time_since_boot_ms();

	for (uint16_t i = 0; i < store->capacity; i++) {
		if (store->cache[i]) {
			free_entry(store->cache[i]);
			store->cache[i] = NULL;
		}
	}

	char filepath[FILEPATH_MAXLEN + FILENAME_MAXLEN];
	get_filepath(filepath, sizeof(filepath), store->ns, NULL, NULL);
	memset(store->dir, 0, sizeof(store->dir));

	info("Clearing UID store: %s", filepath);
	int err = fs_dir(store->fs, filepath, on_clear_dir, store);

	const uint32_t elapsed = board_get_time_since_boot_ms() - t0;
	metrics_set_if_max(UIDClearTimeMax, METRICS_VALUE(elapsed));
	metrics_set_if_min(UIDClearTimeMin, METRICS_VALUE(elapsed));

	return err;
}

int uid_register_update_cb(struct uid_store *store,
		uid_update_cb_t cb, void *ctx)
{
	store->cb = cb;
	store->cb_ctx = ctx;
	return 0;
}

struct uid_store *uid_store_create(const struct uid_store_config *config)
{
	if (!config || !config->fs || !config->ns) {
		return NULL;
	}

	struct uid_store *store = (struct uid_store *)calloc(1, sizeof(*store));

	if (store) {
		store->fs = config->fs;
		store->ns = config->ns;
		store->capacity = config->capacity;
		store->cache = (struct cache_entry **)calloc(1,
			config->capacity * sizeof(struct cache_entry *));
		if (!store->cache) {
			error("Failed to allocate memory for UID cache.");
			free(store);
			store = NULL;
		}
	}

	return store;
}

void uid_store_destroy(struct uid_store *store)
{
	if (store) {
		for (uint16_t i = 0; i < store->capacity; i++) {
			if (store->cache[i]) {
				free_entry(store->cache[i]);
			}
		}
		free(store->cache);
		free(store);
	}
}
