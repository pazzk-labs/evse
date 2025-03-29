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

#include "safety.h"

#include <stdlib.h>
#include <errno.h>

#include "libmcu/list.h"

struct safety {
	struct list entries;
};

struct entry_container {
	struct list link;
	struct safety_entry *entry;
};

struct check_ctx {
	safety_error_callback_t cb;
	void *cb_ctx;
	int errcnt;
};

static struct entry_container *find_entry_container(struct safety *self,
		struct safety_entry *entry)
{
	struct list *p;

	list_for_each(p, &self->entries) {
		struct entry_container *container =
			list_entry(p, struct entry_container, link);
		if (container->entry == entry) {
			return container;
		}
	}

	return NULL;
}

static int add_entry(struct safety *self, struct safety_entry *entry)
{
	struct entry_container *container;

	if (!entry) {
		return -EINVAL;
	}

	if (find_entry_container(self, entry)) {
		return -EALREADY;
	}

	if (!(container = malloc(sizeof(struct entry_container)))) {
		return -ENOMEM;
	}

	container->entry = entry;
	list_add_tail(&container->link, &self->entries);

	return 0;
}

static int remove_entry(struct safety *self, struct safety_entry *entry)
{
	struct entry_container *container = find_entry_container(self, entry);

	if (!container) {
		return -ENOENT;
	}

	list_del(&container->link, &self->entries);
	free(container);

	return 0;
}

static void iterate(struct safety *self, safety_iterator_t cb, void *cb_ctx)
{
	struct list *p;
	struct list *n;

	list_for_each_safe(p, n, &self->entries) {
		struct entry_container *container =
			list_entry(p, struct entry_container, link);
		if (cb) {
			(*cb)(container->entry, cb_ctx);
		}
	}
}

static void on_safety_check(struct safety_entry *entry, void *ctx)
{
	struct check_ctx *p = (struct check_ctx *)ctx;
	const safety_entry_status_t status = safety_entry_check(entry);

	if (status != SAFETY_STATUS_OK) {
		if (p->cb) {
			(*p->cb)(entry, status, p->cb_ctx);
		}
		p->errcnt++;
	}
}

int safety_check(struct safety *self, safety_error_callback_t cb, void *cb_ctx)
{
	struct check_ctx ctx = {
		.cb = cb,
		.cb_ctx = cb_ctx,
		.errcnt = 0,
	};

	if (!self) {
		return -EINVAL;
	}

	iterate(self, on_safety_check, &ctx);

	return ctx.errcnt;
}

int safety_iterate(struct safety *self, safety_iterator_t cb, void *cb_ctx)
{
	if (!self || !cb) {
		return -EINVAL;
	}

	iterate(self, cb, cb_ctx);

	return 0;
}

int safety_add(struct safety *self, struct safety_entry *entry)
{
	return add_entry(self, entry);
}

int safety_add_and_enable(struct safety *self, struct safety_entry *entry)
{
	int err = add_entry(self, entry);

	if (!err) {
		if ((err = safety_entry_enable(entry)) != 0) {
			remove_entry(self, entry);
			err = -EIO;
		}
	}

	return err;
}

int safety_remove(struct safety *self, struct safety_entry *entry)
{
	if (!self || !entry) {
		return -EINVAL;
	}

	return remove_entry(self, entry);
}

struct safety *safety_create(void)
{
	struct safety *self;

	if (!(self = malloc(sizeof(struct safety)))) {
		return NULL;
	}

	list_init(&self->entries);

	return self;
}

void safety_destroy(struct safety *self)
{
	struct list *p, *n;

	if (!self) {
		return;
	}

	list_for_each_safe(p, n, &self->entries) {
		struct entry_container *container =
			list_entry(p, struct entry_container, link);
		list_del(&container->link, &self->entries);

		safety_entry_disable(container->entry);
		safety_entry_destroy(container->entry);

		free(container);
	}

	free(self);
}
