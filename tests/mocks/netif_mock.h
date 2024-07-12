/*
 * This file is part of PAZZK EVSE project <https://pazzk.net/>.
 * Copyright (c) 2024 권경환 Kyunghwan Kwon <k@libmcu.org>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NETIF_MOCK_H
#define NETIF_MOCK_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "net/netif.h"

struct netif *mock_netif_create(void);
void mock_netif_raise_event(struct netif *netif, const netif_event_t event);
bool mock_netif_enabled(struct netif *netif);

#if defined(__cplusplus)
}
#endif

#endif /* APP_APP_HH */
