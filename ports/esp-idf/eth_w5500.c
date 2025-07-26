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

#include "net/eth_w5500.h"

#include <errno.h>
#include <string.h>

#include "esp_eth_mac.h"
#include "esp_eth_mac_spi.h"
#include "esp_eth_driver.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_eth_netif_glue.h"
#include "esp_event.h"
#include "lwip/ip_addr.h"

#include "pinmap_gpio.h"
#include "logger.h"

#if !defined(NETIF_CALLBACK_MAX)
#define NETIF_CALLBACK_MAX		1
#endif
#if !defined(NETIF_DEFAULT_DNS_SERVER)
#define NETIF_DEFAULT_DNS_SERVER	"1.1.1.1"
#endif

/* NOTE: This is a workaround for the issue that esp_netif_deinit() is not
 * supported in the current ESP-IDF version. We do not deinit the netif at all
 * and skip init and install driver if re-init is requested. */
#define NETIF_DEINIT_NOT_SUPPORTED

struct callback {
	netif_event_callback_t cb;
	void *cb_ctx;
	netif_event_t event_type;
};

struct netif {
	struct netif_api api;
	const char *name;

	struct callback callbacks[NETIF_CALLBACK_MAX];

	esp_netif_t *iface;
	esp_eth_handle_t handle;
	uint8_t mac_addr[NETIF_MAC_ADDR_LEN];
	lm_ip_info_t static_ip;
	lm_ip_info_t ip_info;
	esp_eth_netif_glue_handle_t glue;
};

static bool is_cb_free(struct callback *cb)
{
	struct callback empty = { 0, };
	return memcmp(cb, &empty, sizeof(empty)) == 0;
}

static struct callback *alloc_cb(struct callback *list,
		const netif_event_t type)
{
	for (int i = 0; i < NETIF_CALLBACK_MAX; i++) {
		struct callback *cb = &list[i];
		if (is_cb_free(cb)) {
			cb->event_type = type;
			return cb;
		}
	}

	return NULL;
}

static void free_cb(struct callback *cb)
{
	memset(cb, 0, sizeof(*cb));
}

static void dispatch_event(struct netif *netif, netif_event_t event_type)
{
	for (int i = 0; i < NETIF_CALLBACK_MAX; i++) {
		struct callback *cb = &netif->callbacks[i];
		if (cb->cb && (cb->event_type == event_type ||
				cb->event_type == NETIF_EVENT_ANY)) {
			cb->cb(netif, event_type, cb->cb_ctx);
		}
	}
}

static void set_dns_server(esp_netif_t *netif,
		const uint32_t addr, const esp_netif_dns_type_t type)
{
	if (addr && (addr != IPADDR_NONE)) {
		esp_netif_dns_info_t dns = {
			.ip = {
				.type = IPADDR_TYPE_V4,
				.u_addr = {
					.ip4 = {
						.addr = addr,
					},
				},
			},
		};
		esp_netif_set_dns_info(netif, type, &dns);
	}
}

static void set_static_ip_if_required(struct netif *netif)
{
	lm_ip_info_t empty = { 0, };

	if (memcmp(&netif->static_ip, &empty, sizeof(empty)) == 0) {
		return;
	}

	if (esp_netif_dhcpc_stop(netif->iface) != ESP_OK) {
		error("Failed to stop dhcpc");
		return;
	}

	char ip_str[16];
	char netmask_str[16];
	char gw_str[16];

	sprintf(ip_str, "%d.%d.%d.%d",
			netif->static_ip.v4.ip.addr[0],
			netif->static_ip.v4.ip.addr[1],
			netif->static_ip.v4.ip.addr[2],
			netif->static_ip.v4.ip.addr[3]);
	sprintf(netmask_str, "%d.%d.%d.%d",
			netif->static_ip.v4.netmask.addr[0],
			netif->static_ip.v4.netmask.addr[1],
			netif->static_ip.v4.netmask.addr[2],
			netif->static_ip.v4.netmask.addr[3]);
	sprintf(gw_str, "%d.%d.%d.%d",
			netif->static_ip.v4.gateway.addr[0],
			netif->static_ip.v4.gateway.addr[1],
			netif->static_ip.v4.gateway.addr[2],
			netif->static_ip.v4.gateway.addr[3]);

	const esp_netif_ip_info_t ip = {
		.ip.addr = ipaddr_addr(ip_str),
		.netmask.addr = ipaddr_addr(netmask_str),
		.gw.addr = ipaddr_addr(gw_str),
	};

	if (esp_netif_set_ip_info(netif->iface, &ip) != ESP_OK) {
		error("Failed to set ip info");
		return;
	}

	set_dns_server(netif->iface, ipaddr_addr(NETIF_DEFAULT_DNS_SERVER),
			ESP_NETIF_DNS_MAIN);
}

static void on_eth_event(void *arg, esp_event_base_t event_base,
		int32_t event_id, void *event_data)
{
	unused(event_base);
	unused(event_data);

	struct netif *netif = (struct netif *)arg;
	netif_event_t netif_event = NETIF_EVENT_UNKNOWN;

	switch (event_id) {
	case ETHERNET_EVENT_CONNECTED:
		set_static_ip_if_required(netif);
		netif_event = NETIF_EVENT_CONNECTED;
		break;
	case ETHERNET_EVENT_DISCONNECTED:
		memset(&netif->ip_info, 0, sizeof(netif->ip_info));
		netif_event = NETIF_EVENT_DISCONNECTED;
		break;
	case ETHERNET_EVENT_START:
		netif_event = NETIF_EVENT_STARTED;
		break;
	case ETHERNET_EVENT_STOP:
		netif_event = NETIF_EVENT_STOPPED;
		break;
	default:
		break;
	}

	dispatch_event(netif, netif_event);
}

static void on_ip_event(void *arg, esp_event_base_t event_base,
		int32_t event_id, void *event_data)
{
	unused(event_base);
	unused(event_id);

	struct netif *netif = (struct netif *)arg;
	ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
	const esp_netif_ip_info_t *ip_info = &event->ip_info;

	netif->ip_info.v4.ip.addr[0] = (uint8_t)(ip_info->ip.addr >> 0);
	netif->ip_info.v4.ip.addr[1] = (uint8_t)(ip_info->ip.addr >> 8);
	netif->ip_info.v4.ip.addr[2] = (uint8_t)(ip_info->ip.addr >> 16);
	netif->ip_info.v4.ip.addr[3] = (uint8_t)(ip_info->ip.addr >> 24);

	netif->ip_info.v4.netmask.addr[0] = (uint8_t)(ip_info->netmask.addr >> 0);
	netif->ip_info.v4.netmask.addr[1] = (uint8_t)(ip_info->netmask.addr >> 8);
	netif->ip_info.v4.netmask.addr[2] = (uint8_t)(ip_info->netmask.addr >> 16);
	netif->ip_info.v4.netmask.addr[3] = (uint8_t)(ip_info->netmask.addr >> 24);

	netif->ip_info.v4.gateway.addr[0] = (uint8_t)(ip_info->gw.addr >> 0);
	netif->ip_info.v4.gateway.addr[1] = (uint8_t)(ip_info->gw.addr >> 8);
	netif->ip_info.v4.gateway.addr[2] = (uint8_t)(ip_info->gw.addr >> 16);
	netif->ip_info.v4.gateway.addr[3] = (uint8_t)(ip_info->gw.addr >> 24);

	dispatch_event(netif, NETIF_EVENT_IP_ACQUIRED);
}

static int set_ip_info(struct netif *netif, const lm_ip_info_t *ip_info)
{
	memcpy(&netif->static_ip, ip_info, sizeof(*ip_info));
	return 0;
}

static int get_ip_info(struct netif *netif, lm_ip_info_t *ip_info)
{
	memcpy(ip_info, &netif->ip_info, sizeof(*ip_info));
	return 0;
}

static int get_speed(struct netif *netif, uint32_t *kbps)
{
	eth_speed_t speed;
	esp_err_t err = esp_eth_ioctl(netif->handle, ETH_CMD_G_SPEED, &speed);
	*kbps = speed == ETH_SPEED_10M? 10*1000 : 100*1000;
	return err != ESP_OK? -err : 0;
}

static int get_duplex(struct netif *netif, bool *duplex_enabled)
{
	eth_duplex_t duplex;
	esp_err_t err = esp_eth_ioctl(netif->handle,
			ETH_CMD_G_DUPLEX_MODE, &duplex);
	*duplex_enabled = duplex == ETH_DUPLEX_FULL;
	return err != ESP_OK? -err : 0;
}

static int send_eth_frame(struct netif *netif, const void *data,
		const size_t datasize)
{
	union {
		const void *data;
		void *p;
	} t = { .data = data };
	esp_err_t err = esp_eth_transmit(netif->handle, t.p, datasize);
	return err != ESP_OK? -err : 0;
}

static int register_event_callback(struct netif *netif,
			const netif_event_t event_type,
			const netif_event_callback_t cb, void *cb_ctx)
{
	struct callback *new = alloc_cb(netif->callbacks, event_type);

	if (new == NULL) {
		return -ENOMEM;
	}

	new->cb = cb;
	new->cb_ctx = cb_ctx;

	return 0;
}

static int unregister_event_callback(struct netif *netif,
			const netif_event_t event_type,
			const netif_event_callback_t cb)
{
	for (int i = 0; i < NETIF_CALLBACK_MAX; i++) {
		struct callback *p = &netif->callbacks[i];
		if (p->cb == cb && p->event_type == event_type) {
			free_cb(p);
			return 0;
		}
	}

	return -ENOENT;
}

static int connect(struct netif *netif)
{
	esp_err_t err = esp_eth_start(netif->handle);
	return err != ESP_OK? -err : 0;
}

static int disconnect(struct netif *netif)
{
	esp_err_t err = esp_eth_stop(netif->handle);
	return err != ESP_OK? -err : 0;
}

static int enable(struct netif *netif)
{
	esp_err_t err;
	esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();

	if ((netif->iface = esp_netif_get_handle_from_ifkey(cfg.base->if_key))
			== NULL) {
		netif->iface = esp_netif_new(&cfg);
	}
	netif->glue = esp_eth_new_netif_glue(netif->handle);

	err = esp_netif_attach(netif->iface, netif->glue);
	err |= esp_event_handler_register(ETH_EVENT,
			ESP_EVENT_ANY_ID, &on_eth_event, netif);
	err |= esp_event_handler_register(IP_EVENT,
			IP_EVENT_ETH_GOT_IP, &on_ip_event, netif);

	return err != ESP_OK? -err : 0;
}

static int disable(struct netif *netif)
{
	esp_err_t err = esp_event_handler_unregister(IP_EVENT,
			IP_EVENT_ETH_GOT_IP, &on_ip_event);
	err |= esp_event_handler_unregister(ETH_EVENT,
			ESP_EVENT_ANY_ID, &on_eth_event);
	err |= esp_eth_del_netif_glue(netif->glue);
	esp_netif_destroy(netif->iface);

	return err == ESP_OK? 0 : -err;
}

static int install_driver(struct netif *netif)
{
	eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
	eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
	phy_config.reset_gpio_num = -1;

	spi_device_interface_config_t spi_devcfg = {
		.mode = 3,
		.clock_speed_hz = SPI_MASTER_FREQ_10M,
		.queue_size = 20,
		.spics_io_num = PINMAP_SPI2_CS_W5500,
	};

	eth_w5500_config_t w5500_config =
		ETH_W5500_DEFAULT_CONFIG(SPI2_HOST, &spi_devcfg);
	w5500_config.int_gpio_num = PINMAP_W5500_INT;

	esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500_config, &mac_config);
	esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);

	if (!mac || !phy) {
		return -ENOMEM;
	}

	esp_eth_config_t eth_config_spi = ETH_DEFAULT_CONFIG(mac, phy);
	esp_err_t err = esp_eth_driver_install(&eth_config_spi, &netif->handle);
	err |= esp_eth_ioctl(netif->handle,
			ETH_CMD_S_MAC_ADDR, netif->mac_addr);

	return err != ESP_OK? -err : 0;
}

static int init(struct netif *netif, uint8_t mac_addr[NETIF_MAC_ADDR_LEN])
{
	static bool reinit;
	esp_err_t err = ESP_OK;

	memcpy(netif->mac_addr, mac_addr, sizeof(netif->mac_addr));

	if (!reinit) {
		err = install_driver(netif);

		if (err != ESP_OK) {
			return -err;
		}

		if ((err = esp_netif_init()) != ESP_OK) {
			return -err;
		}
	}
#if defined(NETIF_DEINIT_NOT_SUPPORTED)
	reinit = true;
#endif
	return err == ESP_OK? 0 : -err;
}

static int deinit(struct netif *netif)
{
	esp_err_t err = ESP_OK;

#if !defined(NETIF_DEINIT_NOT_SUPPORTED)
	err |= esp_netif_deinit();
	err |= esp_eth_driver_uninstall(netif->handle);
#endif

	for (int i = 0; i < NETIF_CALLBACK_MAX; i++) {
		free_cb(&netif->callbacks[i]);
	}

	return err == ESP_OK? 0 : -err;
}

struct netif *eth_w5500_create(struct lm_spi_device *spi)
{
	unused(spi);

	static struct netif netif = {
		.api = {
			.init = init,
			.deinit = deinit,
			.enable = enable,
			.disable = disable,
			.connect = connect,
			.disconnect = disconnect,
			.register_event_callback = register_event_callback,
			.unregister_event_callback = unregister_event_callback,
			.set_ip_info = set_ip_info,
			.get_ip_info = get_ip_info,
			.get_speed = get_speed,
			.get_duplex = get_duplex,
			.send_eth_frame = send_eth_frame,
		},
		.name = "w5500",
	};

	return &netif;
}
