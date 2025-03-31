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

#include "libmcu/nvs_kvstore.h"

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <pthread.h>
#include <ctype.h>

#include "libmcu/compiler.h"

#if !defined(KVSTORE_MAX_KEY_LENGTH)
#define KVSTORE_MAX_KEY_LENGTH			32
#endif

struct kvstore {
	struct kvstore_api api;
	const char *namespace;
	pthread_mutex_t lock;
};

static void sanitize_key(const char *key, char *out, size_t maxlen)
{
	size_t i, j = 0;
	for (i = 0; key[i] != '\0' && j < maxlen - 1; i++) {
		char c = key[i];
		if (isalnum((unsigned char)c) || c == '-' || c == '_') {
			out[j++] = c;
		}
	}
	out[j] = '\0';
}

static char *kvstore_make_path(const char *ns, const char *key)
{
	static char path[256];
	char sanitized_key[KVSTORE_MAX_KEY_LENGTH];
	sanitize_key(key, sanitized_key, sizeof(sanitized_key));

	snprintf(path, sizeof(path), "./%s/%s.kv", ns, sanitized_key);
	return path;
}

static int file_kvstore_write(struct kvstore *kvstore,
		const char *key, const void *value, size_t size)
{
	pthread_mutex_lock(&kvstore->lock);

	char *filepath = kvstore_make_path(kvstore->namespace, key);
	FILE *fp = fopen(filepath, "wb");
	if (!fp) {
		pthread_mutex_unlock(&kvstore->lock);
		return 0;
	}

	size_t written = fwrite(value, 1, size, fp);
	fclose(fp);

	pthread_mutex_unlock(&kvstore->lock);
	return (int)written;
}

static int file_kvstore_read(struct kvstore *kvstore,
		const char *key, void *buf, size_t bufsize)
{
	pthread_mutex_lock(&kvstore->lock);

	char *filepath = kvstore_make_path(kvstore->namespace, key);
	FILE *fp = fopen(filepath, "rb");
	if (!fp) {
		pthread_mutex_unlock(&kvstore->lock);
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	long fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (fsize <= 0) {
		fclose(fp);
		pthread_mutex_unlock(&kvstore->lock);
		return 0;
	}

	size_t to_read = (size_t)fsize < bufsize ? (size_t)fsize : bufsize;
	size_t read = fread(buf, 1, to_read, fp);
	fclose(fp);

	pthread_mutex_unlock(&kvstore->lock);
	return (int)read;
}

static int file_kvstore_open(struct kvstore *kvstore, const char *ns)
{
	unused(kvstore);

	struct stat st = { 0, };

	if (stat(ns, &st) == -1) {
		if (mkdir(ns, 0700) != 0) {
			return -ENOENT;
		}
	}

	return 0;
}

struct kvstore *nvs_kvstore_new(void)
{
	struct kvstore *kv = malloc(sizeof(*kv));
	if (!kv) {
		return NULL;
	}

	kv->namespace = "build/config";
	if (!kv->namespace) {
		free(kv);
		return NULL;
	}

	if (file_kvstore_open(kv, kv->namespace) < 0) {
		free(kv);
		return NULL;
	}

	if (pthread_mutex_init(&kv->lock, NULL) != 0) {
		free(kv);
		return NULL;
	}

	kv->api.write = file_kvstore_write;
	kv->api.read = file_kvstore_read;
	kv->api.open = file_kvstore_open;

	return kv;
}
