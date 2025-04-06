/*
 * This file is part of the Pazzk project <https://pazzk.net/>.
 * Copyright (c) 2025 Pazzk <team@pazzk.net>.
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

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "charger/ocpp.h"

TEST_GROUP(OcppCharger) {
	struct charger *charger;
	struct charger_param param;

	void setup(void) {
		charger_default_param(&param);
		charger = ocpp_charger_create(NULL);
		charger_init(charger, &param, NULL);
	}
	void teardown(void) {
		charger_deinit(charger);
		ocpp_charger_destroy(charger);

		mock().checkExpectations();
		mock().clear();
	}
};

TEST(OcppCharger, ShouldReturnNullConnector_WhenNoConnectorAttached) {
	LONGS_EQUAL(NULL, ocpp_charger_get_connector_by_tid(charger, 0));
}
TEST(OcppCharger, ShouldReturnPendingRebootType) {
	LONGS_EQUAL(OCPP_CHARGER_REBOOT_NONE,
			ocpp_charger_get_pending_reboot_type(charger));
	ocpp_charger_request_reboot(charger, OCPP_CHARGER_REBOOT_REQUIRED);
	LONGS_EQUAL(OCPP_CHARGER_REBOOT_REQUIRED,
			ocpp_charger_get_pending_reboot_type(charger));
	ocpp_charger_request_reboot(charger, OCPP_CHARGER_REBOOT_REQUIRED_REMOTELY);
	LONGS_EQUAL(OCPP_CHARGER_REBOOT_REQUIRED_REMOTELY,
			ocpp_charger_get_pending_reboot_type(charger));
	ocpp_charger_request_reboot(charger, OCPP_CHARGER_REBOOT_FORCED);
	LONGS_EQUAL(OCPP_CHARGER_REBOOT_FORCED,
			ocpp_charger_get_pending_reboot_type(charger));
}
TEST(OcppCharger, ShouldSaveCheckpoint_WhenSetCheckpoint) {
	const struct ocpp_checkpoint expected = {
		.connector = {
			{.transaction_id = 0x5678, .unavailable = true},
		},
	};
	ocpp_charger_set_checkpoint(charger, &expected);
	struct ocpp_checkpoint *actual = ocpp_charger_get_checkpoint(charger);
	MEMCMP_EQUAL(&expected, actual, sizeof(expected));
}
