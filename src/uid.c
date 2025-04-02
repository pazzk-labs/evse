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

#include "libmcu/compiler.h"
#include "libmcu/hexdump.h"
#include "fs/fs.h"
#include "logger.h"

struct uid {
	uid_id_t id;
	uid_id_t pid;
	time_t expiry;
	uid_status_t status;
} LIBMCU_PACKED;

struct uid_entry {
	struct uid uid;
};

struct cache_entry {
	struct uid_entry entry;
};

static struct {
	struct fs *fs;
	uid_update_cb_t cb;
	void *cb_ctx;

	struct cache_entry **cache;
	uint16_t cache_capacity;
} m;

static uint32_t hash_uid(const uid_id_t id)
{
	uint32_t h = 2166136261u;

	for (size_t i = 0; i < sizeof(uid_id_t); i++) {
		h = (h ^ id[i]) * 16777619u;
	}

	return h % (uint32_t)m.cache_capacity;
}

uid_status_t uid_status(const uid_id_t id, time_t *expiry)
{
	const uint32_t h = hash_uid(id);
	struct cache_entry *entry = m.cache[h];

	if (entry && memcmp(entry->entry.uid.id, id, sizeof(uid_id_t)) == 0) {
		if (expiry) {
			*expiry = entry->entry.uid.expiry;
		}
		return entry->entry.uid.status;
	}

	return UID_STATUS_NO_ENTRY;
}

int uid_update_cache(const uid_id_t id, const uid_id_t pid,
		uid_status_t status, time_t expiry)
{
	const uint32_t h = hash_uid(id);
	struct cache_entry *entry = m.cache[h];

	if (!entry) {
		if ((entry = (struct cache_entry *)calloc(1, sizeof(*entry)))
				== NULL) {
			error("Failed to allocate memory for UID cache.");
			return -ENOMEM;
		}
		m.cache[h] = entry;
	}

	memcpy(entry->entry.uid.id, id, sizeof(uid_id_t));
	memcpy(entry->entry.uid.pid, pid, sizeof(uid_id_t));
	entry->entry.uid.status = status;
	entry->entry.uid.expiry = expiry;

	char buf[sizeof(struct uid)*2+1];
	hexdump(buf, sizeof(buf), id, sizeof(uid_id_t));
	info("UID updated: %s", buf);

	if (m.cb) {
		(*m.cb)(id, status, expiry, m.cb_ctx);
	}

	return 0;
}

int uid_delete(const uid_id_t id)
{
	const uint32_t h = hash_uid(id);
	struct cache_entry *entry = m.cache[h];

	if (entry && memcmp(entry->entry.uid.id, id, sizeof(uid_id_t)) == 0) {
		free(entry);
		m.cache[h] = NULL;
		char buf[sizeof(struct uid)*2+1];
		hexdump(buf, sizeof(buf), id, sizeof(uid_id_t));
		info("UID deleted: %s", buf);
		return 0;
	}

	return -ENOENT;
}

int uid_register_update_cb(uid_update_cb_t cb, void *ctx)
{
	m.cb = cb;
	m.cb_ctx = ctx;
	return 0;
}

int uid_init(struct fs *fs, uint16_t cache_capacity)
{
	memset(&m, 0, sizeof(m));

	m.fs = fs;
	m.cache_capacity = cache_capacity;

	if ((m.cache = (struct cache_entry **)calloc(1, cache_capacity
			* sizeof(struct cache_entry *))) == NULL) {
		error("Failed to allocate memory for UID cache.");
		return -ENOMEM;
	}

	return 0;
}

void uid_deinit(void)
{
	for (uint16_t i = 0; i < m.cache_capacity; i++) {
		if (m.cache[i]) {
			free(m.cache[i]);
		}
	}
	free(m.cache);
}
