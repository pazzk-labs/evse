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

#ifndef OCPP_CHARGER_PRIVATE_H
#define OCPP_CHARGER_PRIVATE_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "charger/ocpp.h"
#include "../charger_private.h"

typedef enum {
	OCPP_CHARGER_REBOOT_NONE,
	OCPP_CHARGER_REBOOT_REQUIRED,
	OCPP_CHARGER_REBOOT_REQUIRED_REMOTELY,
	OCPP_CHARGER_REBOOT_FORCED,
} ocpp_charger_reboot_t;

struct ocpp_charger {
	struct charger base;
	struct ocpp_charger_param param;

	bool param_changed; /* set when availability changed or
			transaction_id acquired */
	bool configuration_changed; /* set when configuration changed */
	bool status_report_required; /* set when conn0 availability changed */
	bool csms_up; /* set when BootNotification.conf: Accepted */
	bool remote_request; /* set when remote reset requested */

	ocpp_charger_reboot_t reboot_required;
};

#if defined(__cplusplus)
}
#endif

#endif /* OCPP_CHARGER_PRIVATE_H */
