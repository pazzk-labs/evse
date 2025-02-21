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

#ifndef NET_UTIL_H
#define NET_UTIL_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#if !defined(MAC_ADDR_LEN)
#define MAC_ADDR_LEN		6
#endif

typedef enum {
	NET_PROTO_UNKNOWN,
	NET_PROTO_HTTP,
	NET_PROTO_HTTPS,
	NET_PROTO_WS,
	NET_PROTO_WSS,
	NET_PROTO_MQTT,
	NET_PROTO_MQTTS,
	NET_PROTO_FTP,
	NET_PROTO_FTPS,
	NET_PROTO_SFTP,
	NET_PROTO_MAX,
} net_protocol_t;

/**
 * @brief Get the network protocol from a URL.
 *
 * This function parses the given URL and returns the corresponding network
 * protocol.
 *
 * @param[in] url The URL to parse.
 *
 * @return The network protocol.
 */
net_protocol_t net_get_protocol_from_url(const char *url);

/**
 * @brief Check if the network protocol is secure.
 *
 * This function checks if the given network protocol is secure (e.g., HTTPS).
 *
 * @param[in] proto The network protocol to check.
 *
 * @return true if the protocol is secure, false otherwise.
 */
bool net_is_secure_protocol(net_protocol_t proto);

/**
 * @brief Validate an IPv4 address string.
 *
 * This function checks if the given string is a valid IPv4 address.
 *
 * @note If the string contains only dots (.) without any numbers, it is
 *       interpreted as 0.
 *
 * @param[in] ipstr The IPv4 address string to validate.
 *
 * @return true if the string is a valid IPv4 address, false otherwise.
 */
bool net_is_ipv4_str_valid(const char *ipstr);

/**
 * @brief Get the MAC address from a string.
 *
 * This function parses the given string and extracts the MAC address.
 *
 * @param[in] str The string containing the MAC address.
 * @param[out] mac The buffer to store the extracted MAC address.
 *
 * @return true if the MAC address was successfully extracted, false otherwise.
 */
bool net_get_mac_from_str(const char *str, uint8_t mac[MAC_ADDR_LEN]);

/**
 * @brief Convert a MAC address to a string.
 *
 * This function converts the given MAC address to a string representation.
 *
 * @param[in] mac The MAC address to convert.
 * @param[out] buf The buffer to store the string representation of the MAC
 *             address.
 * @param[in] bufsize The size of the buffer.
 *
 * @return true if the MAC address was successfully converted, false otherwise.
 */
bool net_stringify_mac(const uint8_t mac[MAC_ADDR_LEN],
		char *buf, size_t bufsize);

#if defined(__cplusplus)
}
#endif

#endif /* NET_UTIL_H */
