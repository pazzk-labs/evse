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

#ifndef ETH_H
#define ETH_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#if defined(LWIP)
#include <lwip/inet.h>
#elif defined(__NEWLIB__)
#include <sys/endian.h>
#define htons		htobe16
#else
#include <arpa/inet.h>
#endif

#if !defined(MAC_ADDR_LEN)
#define MAC_ADDR_LEN		6
#endif

typedef uint16_t eth_etype_t;
enum {
	ETH_ETYPE_IPv4		= 0x0800,
	ETH_ETYPE_ARP		= 0x0806,
	ETH_ETYPE_IPv6		= 0x86DD,
	ETH_ETYPE_HOMEPLUG	= 0x88E1,
};

struct eth {
	uint8_t dst[MAC_ADDR_LEN];
	uint8_t src[MAC_ADDR_LEN];
	uint16_t etype;
	uint8_t payload[];
} __attribute__((packed));

static inline void eth_set_dst(struct eth *eth, const uint8_t dst[6])
{
	memcpy(eth->dst, dst, sizeof(eth->dst));
}

static inline void eth_set_src(struct eth *eth, const uint8_t src[6])
{
	memcpy(eth->src, src, sizeof(eth->src));
}

static inline void eth_set_etype(struct eth *eth, uint16_t etype)
{
	eth->etype = htons(etype);
}

static inline uint16_t eth_get_etype(const struct eth *eth)
{
	return htons(eth->etype);
}

#if defined(__cplusplus)
}
#endif

#endif /* ETH_H */
