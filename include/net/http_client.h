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

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

typedef enum {
	HTTP_GET,
	HTTP_POST,
	HTTP_PUT,
	HTTP_PATCH,
	HTTP_DELETE,
} http_method_t;

struct http_client;
struct http_client_response;

typedef void (*http_client_data_cb_t)
		(const void *data, size_t datasize, void *ctx);

typedef void (*http_client_response_cb_t)
		(const struct http_client_response *response, void *ctx);

struct http_client_param {
	const char *url;

	const uint8_t *ca_cert;
	size_t ca_cert_len;
	const uint8_t *client_cert;
	size_t client_cert_len;
	const uint8_t *client_key;
	size_t client_key_len;

	uint32_t timeout_ms;
	size_t buffer_size;
};

struct http_client_response {
	uint16_t status_code;
};

struct http_client_request {
	http_method_t method;
	const void *body;
	size_t body_size;
};

/**
 * @brief Create an HTTP client instance.
 *
 * This function creates an HTTP client instance with the specified connection
 * parameters, data callback, and callback context.
 *
 * @param[in] param Pointer to the HTTP client connection parameters structure.
 * @param[in] cb Data callback function to be called when data is received.
 * @param[in] cb_ctx Context to be passed to the data callback function.
 *
 * @return struct http_client* Pointer to the created HTTP client instance, or
 *         NULL on failure.
 */
struct http_client *http_client_create(const struct http_client_param *param,
		http_client_data_cb_t cb, void *cb_ctx);

/**
 * @brief Send an HTTP request.
 *
 * This function sends an HTTP request using the specified HTTP client instance.
 *
 * @param[in] self Pointer to the HTTP client instance.
 * @param[in] req Pointer to the HTTP client request structure.
 * @param[in] cb Response callback function to be called when the response is
 *            received.
 * @param[in] cb_ctx Context to be passed to the response callback function.
 *
 * @return int 0 on success, or a negative error code on failure.
 */
int http_client_request(struct http_client *self,
		const struct http_client_request *req,
		http_client_response_cb_t cb, void *cb_ctx);

/**
 * @brief Destroy an HTTP client instance.
 *
 * This function destroys the specified HTTP client instance and releases any
 * associated resources.
 *
 * @param[in] self Pointer to the HTTP client instance to be destroyed.
 *
 * @return int 0 on success, or a negative error code on failure.
 */
int http_client_destroy(struct http_client *self);

#if defined(__cplusplus)
}
#endif

#endif /* HTTP_CLIENT_H */
