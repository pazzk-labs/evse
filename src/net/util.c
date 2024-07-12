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
#include <string.h>

#if !defined(ARRAY_SIZE)
#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))
#endif

struct proto_tbl {
	net_protocol_t proto;
	const char *prefix;
};

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

	for (size_t i = 0; i < ARRAY_SIZE(tbl); i++) {
		if (strncmp(url, tbl[i].prefix, strlen(tbl[i].prefix)) == 0) {
			return tbl[i].proto;
		}
	}

	return NET_PROTO_UNKNOWN;
}
