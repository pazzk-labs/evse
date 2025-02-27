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

#ifndef CHARGER_EVENT_H
#define CHARGER_EVENT_H

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
	CHARGER_EVENT_NONE			= 0x00000000U,
	CHARGER_EVENT_PLUGGED			= 0x00000001U, /* EV connected */
	CHARGER_EVENT_UNPLUGGED			= 0x00000002U, /* EV disconnected */
	CHARGER_EVENT_CHARGING_STARTED		= 0x00000004U, /* C or D state */
	CHARGER_EVENT_CHARGING_SUSPENDED	= 0x00000008U, /* SuspendedEV */
	CHARGER_EVENT_CHARGING_ENDED		= 0x00000010U,
	CHARGER_EVENT_ERROR			= 0x00000020U,
	CHARGER_EVENT_ERROR_RECOVERY		= 0x00000040U,
	CHARGER_EVENT_REBOOT_REQUIRED		= 0x00000080U,
	CHARGER_EVENT_CONFIGURATION_CHANGED	= 0x00000100U, /* OCPP configuration */
	CHARGER_EVENT_PARAM_CHANGED		= 0x00000200U, /* availability or transactionId */
	CHARGER_EVENT_BILLING_STARTED		= 0x00000400U, /* StartTransaction.conf */
	CHARGER_EVENT_BILLING_REALTIME		= 0x00000800U,
	CHARGER_EVENT_BILLING_ENDED		= 0x00001000U, /* StopTransaction.conf */
	CHARGER_EVENT_OCCUPIED			= 0x00002000U, /* occupied but not plugged yet */
	CHARGER_EVENT_UNOCCUPIED		= 0x00004000U, /* time out */
	CHARGER_EVENT_AUTH_REJECTED		= 0x00008000U,
	CHARGER_EVENT_RESERVED			= 0x00010000U,
} charger_event_t;

#if defined(__cplusplus)
}
#endif

#endif /* CHARGER_EVENT_H */
