#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "charger/ocpp.h"

TEST_GROUP(OcppCharger) {
	struct charger *charger;
	struct charger_param param;

	void setup(void) {
		charger_default_param(&param);
		charger = ocpp_charger_create();
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
			{.transaction_id = 0x1234},
			{.transaction_id = 0x5678, .unavailable = true},
		},
	};
	ocpp_charger_set_checkpoint(charger, &expected);
	struct ocpp_checkpoint *actual = ocpp_charger_get_checkpoint(charger);
	MEMCMP_EQUAL(&expected, actual, sizeof(expected));
}
