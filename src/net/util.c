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

#include "net/util.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#if !defined(ARRAY_COUNT)
#define ARRAY_COUNT(x)		(sizeof(x) / sizeof((x)[0]))
#endif

#if !defined(MIN)
#define MIN(a, b)		((a) > (b)? (b) : (a))
#endif

struct proto_tbl {
	net_protocol_t proto;
	const char *prefix;
};

static bool is_hex(char c)
{
	return (c >= '0' && c <= '9') ||
		(c >= 'A' && c <= 'F') ||
		(c >= 'a' && c <= 'f');
}

net_protocol_t net_get_protocol_from_url(const char *url)
{
	const struct proto_tbl tbl[] = {
		{ NET_PROTO_HTTP,  "http://" },
		{ NET_PROTO_HTTPS, "https://" },
		{ NET_PROTO_WS,    "ws://" },
		{ NET_PROTO_WSS,   "wss://" },
		{ NET_PROTO_MQTT,  "mqtt://" },
		{ NET_PROTO_MQTTS, "mqtts://" },
		{ NET_PROTO_FTP,   "ftp://" },
		{ NET_PROTO_FTPS,  "ftps://" },
		{ NET_PROTO_SFTP,  "sftp://" },
	};

	for (size_t i = 0; i < ARRAY_COUNT(tbl); i++) {
		if (strncmp(url, tbl[i].prefix, strlen(tbl[i].prefix)) == 0) {
			return tbl[i].proto;
		}
	}

	return NET_PROTO_UNKNOWN;
}

bool net_is_secure_protocol(net_protocol_t proto)
{
	const bool secure[NET_PROTO_MAX] = {
		[NET_PROTO_HTTPS] = true,
		[NET_PROTO_WSS]   = true,
		[NET_PROTO_MQTTS] = true,
		[NET_PROTO_FTPS]  = true,
		[NET_PROTO_SFTP]  = true,
	};

	return secure[proto];
}

uint16_t net_get_port_from_url(const char *url)
{
	const char *p = strstr(url, "://");
	if (!p) {
		return 0;
	}

	p += 3;
	const char *end = strchr(p, ':');
	if (!end) {
		return 0;
	}

	return (uint16_t)strtol(end + 1, NULL, 10);
}

int net_get_path_from_url(const char *url, char *buf, size_t bufsize)
{
	memset(buf, 0, bufsize);

	const char *p = strstr(url, "://");
	if (!p) {
		return -EINVAL;
	}

	p = strchr(p + 3, '/');
	if (!p) {
		buf[0] = '/';
		return 0;
	}

	strncpy(buf, p, MIN(strlen(p), bufsize - 1));

	return 0;
}

int net_get_host_from_url(const char *url, char *buf, size_t bufsize)
{
	memset(buf, 0, bufsize);

	const net_protocol_t proto = net_get_protocol_from_url(url);

	if (proto == NET_PROTO_UNKNOWN) {
		return -EINVAL;
	}

	const char *p = strstr(url, "://");
	const char *end = strchr(p + 3, ':');
	if (!end) {
		if (!(end = strchr(p + 3, '/'))) {
			end = p + strlen(p);
		}
	}

	strncpy(buf, p + 3, MIN((size_t)(end - p - 3), bufsize - 1));

	return 0;
}

bool net_is_ipv4_str_valid(const char *ipstr)
{
	int i = 0;
	int num = 0;
	int dots = 0;

	while (ipstr[i]) {
		if (ipstr[i] == '.') {
			if (num < 0 || num > 255) {
				return false;
			}
			num = 0;
			dots++;
		} else if (ipstr[i] >= '0' && ipstr[i] <= '9') {
			num = num * 10 + (ipstr[i] - '0');
		} else {
			return false;
		}
		i++;
	}

	if (num < 0 || num > 255 || dots != 3) {
		return false;
	}

	return true;
}

bool net_get_mac_from_str(const char *str, uint8_t mac[MAC_ADDR_LEN])
{
	const size_t len = strlen(str);

	if (len != 17 && len != 12) {
		return false;
	}

	for (size_t i = 0; len == 17 && i < len; i++) { /* XX:XX:XX:XX:XX:XX */
		if (i % 3 == 2) {
			if (str[i] != ':') {
				return false;
			}
		} else {
			if (!is_hex(str[i])) {
				return false;
			}
		}

		if (i < MAC_ADDR_LEN) {
			mac[i] = (uint8_t)strtol(str + i * 3, NULL, 16);
		}
	}

	for (size_t i = 0; len == 12 && i < len; i++) { /* XXXXXXXXXXXX */
		if (!is_hex(str[i])) {
			return false;
		}

		if (i < MAC_ADDR_LEN) {
			char n[3] = { str[i*2], str[i*2+1], '\0' };
			mac[i] = (uint8_t)strtol(n, NULL, 16);
		}
	}

	return true;
}

bool net_stringify_mac(const uint8_t mac[MAC_ADDR_LEN],
		char *buf, size_t bufsize)
{
	return snprintf(buf, bufsize, "%02x:%02x:%02x:%02x:%02x:%02x",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]) == 17;
}
