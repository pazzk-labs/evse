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

#ifndef IP_H
#define IP_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

struct ipv4 {
	uint8_t addr[4];
};

struct ipv6 {
	uint8_t addr[16];
	uint8_t zone;
};

struct ipv4_info {
	struct ipv4 ip;
	struct ipv4 netmask;
	struct ipv4 gateway;
};

union ip {
	struct ipv4 v4;
	struct ipv6 v6;
};

union ip_info {
	struct ipv4_info v4;
	struct ipv6 v6;
};

typedef union ip ip_t;
typedef union ip_info ip_info_t;

#if defined(__cplusplus)
}
#endif

#endif /* IP_H */
