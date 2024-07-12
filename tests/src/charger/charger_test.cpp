#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "charger_private.h"
#include "connector_private.h"

static struct connector *create_connector(struct charger *charger,
			const struct connector_param *param) {
	struct connector *c = (struct connector *)
		mock().actualCall("create_connector")
		.returnPointerValueOrDefault(NULL);
	c->param = *param;
	c->id = 1;
	c->charger = charger;
	list_add_tail(&c->link, &charger->connectors.list);
	return c;
}

static void destroy_connector(struct charger *charger, struct list *link) {
}

TEST_GROUP(Charger) {
	struct charger charger;
	struct connector c;

	void setup(void) {
		memset(&charger, 0, sizeof(charger));
		memset(&c, 0, sizeof(c));
		struct charger_param param = {
			.max_input_current_mA = 50000,
			.input_voltage = 220,
			.input_frequency = 60,
		};
		charger.api = (struct charger_api) {
			.create_connector = create_connector,
			.destroy_connector = destroy_connector,
		};

		charger_init(&charger, &param);
	}
	void teardown(void) {
		charger_deinit(&charger);

		mock().checkExpectations();
		mock().clear();
	}

	void attach_connector(void) {
		struct connector_param param = {
			.max_output_current_mA = 5,
			.min_output_current_mA = 0,
			.iec61851 = (struct iec61851 *)1,
			.name = "name",
		};
		mock().expectOneCall("create_connector").andReturnValue(&c);
		charger_create_connector(&charger, &param);
	}
};

TEST(Charger, ShouldReturnEINVAL_WhenNullParamGiven) {
	LONGS_EQUAL(-EINVAL, charger_init(&charger, NULL));

	struct charger_param param = {
		.max_input_current_mA = 50000,
		.input_voltage = 0,
		.input_frequency = 60,
	};
	charger_init(&charger, &param);
}

TEST(Charger, ShouldCallCreateConnector_WhenValidParamGiven) {
	struct connector_param param = {
		.max_output_current_mA = 5,
		.min_output_current_mA = 0,
		.iec61851 = (struct iec61851 *)1,
		.name = "name",
	};
	mock().expectOneCall("create_connector").andReturnValue(&c);
	charger_create_connector(&charger, &param);
}

TEST(Charger, ShouldReturnConnector_WhenConnectorNameGiven) {
	struct connector_param param = {
		.max_output_current_mA = 5,
		.min_output_current_mA = 0,
		.iec61851 = (struct iec61851 *)1,
		.name = "name",
	};
	mock().expectOneCall("create_connector").andReturnValue(&c);
	charger_create_connector(&charger, &param);
	LONGS_EQUAL(&c, charger_get_connector(&charger, "name"));
}

TEST(Charger, ShouldReturnConnector_WhenConnectorIdGiven) {
	struct connector_param param = {
		.max_output_current_mA = 5,
		.min_output_current_mA = 0,
		.iec61851 = (struct iec61851 *)1,
		.name = "name",
	};
	mock().expectOneCall("create_connector").andReturnValue(&c);
	charger_create_connector(&charger, &param);
	LONGS_EQUAL(&c, charger_get_connector_by_id(&charger, 1));
}

TEST(Charger, get_connector_ShouldReturnNull_WhenNoConnectorRegistered) {
	LONGS_EQUAL(NULL, charger_get_connector(&charger, "name"));
}

TEST(Charger, ShouldSetPriority) {
	LONGS_EQUAL(0, connector_set_priority(&c, 10));
	LONGS_EQUAL(10, c.param.priority);
}

TEST(Charger, ShouldReturnFalse_WhenNullParamGiven) {
	LONGS_EQUAL(false, connector_validate_param(NULL, 0));
}
TEST(Charger, ShouldReturnFalse_WhenNullNameGiven) {
	struct connector_param param = {
		.name = NULL,
	};
	LONGS_EQUAL(false, connector_validate_param(&param, 0));
}
TEST(Charger, ShouldReturnFalse_WhenNullIec61851Given) {
	struct connector_param param = {
		.iec61851 = NULL,
		.name = "name",
	};
	LONGS_EQUAL(false, connector_validate_param(&param, 0));
}
TEST(Charger, ShouldReturnFalse_WhenMinOutputCurrentGreaterThanMaxOutputCurrent) {
	struct connector_param param = {
		.max_output_current_mA = 5,
		.min_output_current_mA = 10,
		.iec61851 = (struct iec61851 *)1,
		.name = "name",
	};
	LONGS_EQUAL(false, connector_validate_param(&param, 0));
}
TEST(Charger, ShouldReturnFalse_WhenMaxOutputCurrentGreaterThanMaxInputCurrent) {
	struct connector_param param = {
		.max_output_current_mA = 10,
		.min_output_current_mA = 5,
		.iec61851 = (struct iec61851 *)1,
		.name = "name",
	};
	LONGS_EQUAL(false, connector_validate_param(&param, 5));
}
TEST(Charger, ShouldReturnTrue_WhenValidParamGiven) {
	struct connector_param param = {
		.max_output_current_mA = 5,
		.min_output_current_mA = 0,
		.iec61851 = (struct iec61851 *)1,
		.name = "name",
	};
	LONGS_EQUAL(true, connector_validate_param(&param, 5));
}

TEST(Charger, ShouldReturnString_WhenStateGiven) {
	STRCMP_EQUAL("A", stringify_state(A));
	STRCMP_EQUAL("B", stringify_state(B));
	STRCMP_EQUAL("C", stringify_state(C));
	STRCMP_EQUAL("D", stringify_state(D));
	STRCMP_EQUAL("E", stringify_state(E));
	STRCMP_EQUAL("F", stringify_state(F));
}
TEST(Charger, ShouldReturnNoneEvent_WhenStateChangedFromEToA) {
	mock().expectOneCall("iec61851_is_occupied_state")
		.ignoreOtherParameters().andReturnValue(false);
	LONGS_EQUAL(CHARGER_EVENT_NONE, get_event_from_state_change(A, E));
}
TEST(Charger, ShouldReturnPluggedEvent_WhenStateChangedFromAToB) {
	mock().expectOneCall("iec61851_is_occupied_state")
		.ignoreOtherParameters().andReturnValue(true);
	mock().expectOneCall("iec61851_is_charging_state")
		.ignoreOtherParameters().andReturnValue(false);
	LONGS_EQUAL(CHARGER_EVENT_PLUGGED, get_event_from_state_change(B, A));
}
TEST(Charger, ShouldReturnErrorEvent_WhenStateChangedFromAToF) {
	mock().expectOneCall("iec61851_is_occupied_state")
		.ignoreOtherParameters().andReturnValue(false);
	LONGS_EQUAL(CHARGER_EVENT_ERROR, get_event_from_state_change(F, A));
}
TEST(Charger, ShouldReturnUnpluggedEvent_WhenStateChangedFromBToA) {
	mock().expectOneCall("iec61851_is_occupied_state")
		.ignoreOtherParameters().andReturnValue(true);
	mock().expectOneCall("iec61851_is_charging_state")
		.ignoreOtherParameters().andReturnValue(false);
	LONGS_EQUAL(CHARGER_EVENT_UNPLUGGED, get_event_from_state_change(A, B));
}
TEST(Charger, ShouldReturnErrorEvent_WhenStateChangedFromBToF) {
	mock().expectOneCall("iec61851_is_occupied_state")
		.ignoreOtherParameters().andReturnValue(true);
	mock().expectOneCall("iec61851_is_charging_state")
		.ignoreOtherParameters().andReturnValue(false);
	LONGS_EQUAL(CHARGER_EVENT_ERROR, get_event_from_state_change(F, B));
}
TEST(Charger, ShouldReturnChargingStartedEvent_WhenStateChangedFromBToC) {
	mock().expectOneCall("iec61851_is_occupied_state")
		.ignoreOtherParameters().andReturnValue(true);
	mock().expectOneCall("iec61851_is_charging_state")
		.ignoreOtherParameters().andReturnValue(false);
	LONGS_EQUAL(CHARGER_EVENT_CHARGING_STARTED,
			get_event_from_state_change(C, B));
}
TEST(Charger, ShouldReturnChargingSuspendedEvent_WhenStateChangedFromCToB) {
	mock().expectOneCall("iec61851_is_occupied_state")
		.ignoreOtherParameters().andReturnValue(true);
	mock().expectOneCall("iec61851_is_charging_state")
		.ignoreOtherParameters().andReturnValue(true);
	LONGS_EQUAL(CHARGER_EVENT_CHARGING_SUSPENDED,
			get_event_from_state_change(B, C));
}
TEST(Charger, ShouldReturnUnpluggedEvent_WhenStateChangedFromCToA) {
	mock().expectOneCall("iec61851_is_occupied_state")
		.ignoreOtherParameters().andReturnValue(true);
	mock().expectOneCall("iec61851_is_charging_state")
		.ignoreOtherParameters().andReturnValue(true);
	LONGS_EQUAL(CHARGER_EVENT_CHARGING_ENDED | CHARGER_EVENT_UNPLUGGED,
			get_event_from_state_change(A, C));
}
TEST(Charger, ShouldReturnErrorEvent_WhenStateChangedFromCToF) {
	mock().expectOneCall("iec61851_is_occupied_state")
		.ignoreOtherParameters().andReturnValue(true);
	mock().expectOneCall("iec61851_is_charging_state")
		.ignoreOtherParameters().andReturnValue(true);
	LONGS_EQUAL(CHARGER_EVENT_ERROR | CHARGER_EVENT_CHARGING_ENDED,
			get_event_from_state_change(F, C));
}
TEST(Charger, ShouldReturnRecoveryEvent_WhenStateChangedFromFToA) {
	mock().expectOneCall("iec61851_is_occupied_state")
		.ignoreOtherParameters().andReturnValue(false);
	LONGS_EQUAL(CHARGER_EVENT_ERROR_RECOVERY,
			get_event_from_state_change(A, F));
}
TEST(Charger, ShouldReturnRecoveryEvent_WhenStateChangedFromFToB) {
	mock().expectOneCall("iec61851_is_occupied_state")
		.ignoreOtherParameters().andReturnValue(false);
	LONGS_EQUAL(CHARGER_EVENT_ERROR_RECOVERY | CHARGER_EVENT_PLUGGED,
			get_event_from_state_change(B, F));
}

TEST(Charger, ShouldReturnFalse_WhenDutyIs100) {
	mock().expectOneCall("iec61851_get_pwm_duty_set").andReturnValue(100);
	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_B);
	LONGS_EQUAL(false, is_state_x2(&c, B));
}
TEST(Charger, ShouldReturnFalse_WhenDutyIs0) {
	mock().expectOneCall("iec61851_get_pwm_duty_set").andReturnValue(0);
	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_B);
	LONGS_EQUAL(false, is_state_x2(&c, B));
}
TEST(Charger, ShouldReturnTrue_WhenDutyIsNominal) {
	mock().expectOneCall("iec61851_get_pwm_duty_set").andReturnValue(53);
	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_B);
	LONGS_EQUAL(true, is_state_x2(&c, B));
}
