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
	uint32_t (*downtime)(const struct server *self);
};

/**
 * @brief Enable the server.
 *
 * This function activates the server, allowing it to start processing
 * requests. It ensures that the server is properly initialized and
 * ready for operation.
 *
 * @param[in] self Pointer to the server structure.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static inline int server_enable(struct server *self) {
	return ((struct server_api *)self)->enable(self);
}

/**
 * @brief Disable the server.
 *
 * This function deactivates the server, stopping it from processing
 * any further requests. It ensures that the server is properly
 * shut down and transitions to an inactive state.
 *
 * @param[in] self Pointer to the server structure.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static inline int server_disable(struct server *self) {
	return ((struct server_api *)self)->disable(self);
}

/**
 * @brief Establish a connection to the server.
 *
 * This function attempts to connect the server to its designated
 * network or endpoint. It ensures that the server is in a connected
 * state and ready to handle requests.
 *
 * @param[in] self Pointer to the server structure.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static inline int server_connect(struct server *self) {
	return ((struct server_api *)self)->connect(self);
}

/**
 * @brief Disconnect the server.
 *
 * This function disconnects the server from its network or endpoint,
 * transitioning it to an offline state. It ensures that all resources
 * related to the connection are properly released.
 *
 * @param[in] self Pointer to the server structure.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static inline int server_disconnect(struct server *self) {
	return ((struct server_api *)self)->disconnect(self);
}

/**
 * @brief Send data through the server.
 *
 * This function sends the specified data through the server's
 * connection. It ensures that the data is transmitted to the
 * intended recipient.
 *
 * @param[in] self Pointer to the server structure.
 * @param[in] data Pointer to the data to be sent.
 * @param[in] datasize Size of the data to be sent, in bytes.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static inline int server_send(struct server *self,
		const void *data, const size_t datasize) {
	return ((struct server_api *)self)->send(self, data, datasize);
}

/**
 * @brief Receive data from the server.
 *
 * This function receives data from the server's connection and stores
 * it in the provided buffer. It ensures that the received data does
 * not exceed the buffer size.
 *
 * @param[in] self Pointer to the server structure.
 * @param[out] buf Pointer to the buffer where received data will be stored.
 * @param[in] bufsize Size of the buffer, in bytes.
 *
 * @return Number of bytes received on success, or a negative error code on
 *         failure.
 */
static inline int server_recv(struct server *self,
		void *buf, const size_t bufsize) {
	return ((struct server_api *)self)->recv(self, buf, bufsize);
}

/**
 * @brief Check if the server is connected.
 *
 * This function determines whether the server is currently connected
 * by evaluating its internal state.
 *
 * @param[in] self Pointer to the server structure.
 *
 * @return true if the server is connected, false otherwise.
 */
static inline bool server_connected(const struct server *self) {
	return ((const struct server_api *)self)->connected(self);
}

/**
 * @brief Calculate the downtime of the server.
 *
 * This function computes the duration for which the server has been
 * continuously offline. If the server is currently online, the
 * downtime is reset and this function will return 0.
 *
 * @param[in] self Pointer to the server structure.
 *
 * @return The continuous downtime of the server in seconds.
 */
static inline uint32_t server_downtime(const struct server *self) {
	return ((const struct server_api *)self)->downtime(self);
}

#if defined(__cplusplus)
}
#endif

#endif /* SERVER_H */
