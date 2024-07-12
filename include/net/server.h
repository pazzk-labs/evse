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

#ifndef SERVER_H
#define SERVER_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct server;

struct server_api {
	int (*enable)(struct server *self);
	int (*disable)(struct server *self);
	int (*connect)(struct server *self);
	int (*disconnect)(struct server *self);
	int (*send)(struct server *self,
			const void *data, const size_t datasize);
	int (*recv)(struct server *self, void *buf, const size_t bufsize);
	bool (*connected)(const struct server *self);
};

static inline int server_enable(struct server *self) {
	return ((struct server_api *)self)->enable(self);
}

static inline int server_disable(struct server *self) {
	return ((struct server_api *)self)->disable(self);
}

static inline int server_connect(struct server *self) {
	return ((struct server_api *)self)->connect(self);
}

static inline int server_disconnect(struct server *self) {
	return ((struct server_api *)self)->disconnect(self);
}

static inline int server_send(struct server *self,
		const void *data, const size_t datasize) {
	return ((struct server_api *)self)->send(self, data, datasize);
}

static inline int server_recv(struct server *self,
		void *buf, const size_t bufsize) {
	return ((struct server_api *)self)->recv(self, buf, bufsize);
}

static inline bool server_connected(const struct server *self) {
	return ((const struct server_api *)self)->connected(self);
}

#if defined(__cplusplus)
}
#endif

#endif /* SERVER_H */
