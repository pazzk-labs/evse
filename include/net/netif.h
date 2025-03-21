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

#ifndef NETIF_H
#define NETIF_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "net/ip.h"

#define NETIF_MAC_ADDR_LEN	6

typedef enum {
	NETIF_EVENT_UNKNOWN,
	NETIF_EVENT_CONNECTED,
	NETIF_EVENT_DISCONNECTED,
	NETIF_EVENT_IP_ACQUIRED,
	NETIF_EVENT_SCAN_RESULT,
	NETIF_EVENT_SCAN_DONE,
	NETIF_EVENT_STARTED,
	NETIF_EVENT_STOPPED,
	NETIF_EVENT_ANY,
} netif_event_t;

struct netif;

typedef void (*netif_event_callback_t)(struct netif *netif,
		const netif_event_t event, void *ctx);

struct netif_api {
	int (*init)(struct netif *netif, uint8_t mac_addr[NETIF_MAC_ADDR_LEN]);
	int (*deinit)(struct netif *netif);
	int (*enable)(struct netif *netif);
	int (*disable)(struct netif *netif);
	int (*connect)(struct netif *netif);
	int (*disconnect)(struct netif *netif);
	int (*register_event_callback)(struct netif *netif,
			const netif_event_t event_type,
			const netif_event_callback_t cb, void *cb_ctx);
	int (*unregister_event_callback)(struct netif *netif,
			const netif_event_t event_type,
			const netif_event_callback_t cb);
	int (*set_ip_info)(struct netif *netif, const ip_info_t *ip_info);
	int (*get_ip_info)(struct netif *netif, ip_info_t *ip_info);
	int (*get_speed)(struct netif *netif, uint32_t *kbps);
	int (*get_duplex)(struct netif *netif, bool *duplex_enabled);
	int (*send_eth_frame)(struct netif *netif, const void *data,
			const size_t datasize);
};

#if defined(__cplusplus)
}
#endif

#endif /* NETIF_H */
