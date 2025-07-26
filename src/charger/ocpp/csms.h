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

#ifndef OCPP_CSMS_H
#define OCPP_CSMS_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "ocpp/ocpp.h"

#if !defined(CSMS_DOWNTIME_MAX_SEC)
#define CSMS_REBOOT_TRIGGER_DOWNTIME_SEC	(60 * 60 * 24U) /* 1 day */
#endif

int csms_init(void *ctx);
int csms_request(const ocpp_message_t msgtype, void *ctx, void *opt);
int csms_request_defer(const ocpp_message_t msgtype, void *ctx, void *opt,
		const uint32_t delay_sec);
int csms_response(const ocpp_message_t msgtype,
		const struct ocpp_message *req, void *ctx, void *opt);
bool csms_is_up(void);
int csms_reconnect(const uint32_t delay_sec);
uint32_t csms_downtime(void);

#if defined(__cplusplus)
}
#endif

#endif /* OCPP_CSMS_H */
