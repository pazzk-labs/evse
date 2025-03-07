#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "charger_internal.h"
#include "charger/connector.h"

TEST_GROUP(Charger) {
	struct charger charger;
	struct charger_param param;

	void setup(void) {
		charger_default_param(&param);
		charger_init(&charger, &param, NULL);
	}
	void teardown(void) {
		charger_deinit(&charger);

		mock().checkExpectations();
		mock().clear();
	}
};

TEST(Charger, ShouldReturnDefaultParam) {
	charger_default_param(&param);

	LONGS_EQUAL(31818, param.max_input_current_mA);
	LONGS_EQUAL(220, param.input_voltage);
	LONGS_EQUAL(60, param.input_frequency);
	LONGS_EQUAL(31818, param.max_output_current_mA);
	LONGS_EQUAL(31818, param.min_output_current_mA);
}
TEST(Charger, ShouldReturnEINVAL_WhenInvalidParamGiven) {
	charger_default_param(&param);
	param.max_input_current_mA = 0;
	LONGS_EQUAL(-EINVAL, charger_init(&charger, &param, NULL));

	charger_default_param(&param);
	param.input_voltage = 0;
	LONGS_EQUAL(-EINVAL, charger_init(&charger, &param, NULL));

	charger_default_param(&param);
	param.input_frequency = 0;
	LONGS_EQUAL(-EINVAL, charger_init(&charger, &param, NULL));
}
TEST(Charger, ShouldReturnZeroConnectors_WhenJustCreated) {
	LONGS_EQUAL(0, charger_count_connectors(&charger));
}
TEST(Charger, ShouldReturnNull_WhenNoConnectorAttached) {
	LONGS_EQUAL(NULL, charger_get_connector_by_id(&charger, 0));
	LONGS_EQUAL(NULL, charger_get_connector_available(&charger));
	LONGS_EQUAL(NULL, charger_get_connector(&charger, ""));
	LONGS_EQUAL(NULL, charger_get_connector(&charger, NULL));
}
TEST(Charger, ShouldAttachConnector) {
	struct connector c;
	int id;
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c));
	LONGS_EQUAL(1, charger_count_connectors(&charger));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c, &id));
	LONGS_EQUAL(1, id);
}
TEST(Charger, ShouldReturnConnector_WhenConnectorIdGiven) {
	struct connector c;
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c));
	LONGS_EQUAL(&c, charger_get_connector_by_id(&charger, 1));
}
TEST(Charger, ShouldReturnAvailableConnector_WhenAvailable) {
	struct connector c;
	mock().expectOneCall("connector_is_enabled").andReturnValue(true);
	mock().expectOneCall("connector_is_reserved").andReturnValue(false);
	mock().expectOneCall("connector_state").andReturnValue(A);
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c));
	LONGS_EQUAL(&c, charger_get_connector_available(&charger));
}
TEST(Charger, ShouldDetachConnector) {
	struct connector c;
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c));
	LONGS_EQUAL(1, charger_count_connectors(&charger));
	LONGS_EQUAL(0, charger_detach_connector(&charger, &c));
	LONGS_EQUAL(0, charger_count_connectors(&charger));
}
TEST(Charger, ShouldReturnCountOfConnectors_WhenConnectorAttached) {
	struct connector c1;
	struct connector c2;
	struct connector c3;
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c1));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c2));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c3));
	LONGS_EQUAL(3, charger_count_connectors(&charger));
}
TEST(Charger, ShouldReturnUniqueConnectorID_WhenMultipleConnectorsAttached) {
	struct connector c1;
	struct connector c2;
	struct connector c3;
	int id1, id2, id3;
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c1));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c2));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c3));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c1, &id1));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c2, &id2));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c3, &id3));
	LONGS_EQUAL(1, id1);
	LONGS_EQUAL(2, id2);
	LONGS_EQUAL(3, id3);
}
TEST(Charger, ShouldReturnUniqueConnectorID_WhenMultipleConnectorsAttachedAndDetached) {
	struct connector c1;
	struct connector c2;
	struct connector c3;
	int id1, id2, id3;
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c1));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c2));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c3));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c1, &id1));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c2, &id2));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c3, &id3));
	LONGS_EQUAL(1, id1);
	LONGS_EQUAL(2, id2);
	LONGS_EQUAL(3, id3);
	LONGS_EQUAL(0, charger_detach_connector(&charger, &c2));
	LONGS_EQUAL(2, charger_count_connectors(&charger));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c2));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c2, &id2));
	LONGS_EQUAL(4, id2);
	LONGS_EQUAL(3, charger_count_connectors(&charger));
}
TEST(Charger, ShouldProcessAllConnectors) {
	struct connector c1;
	struct connector c2;
	struct connector c3;
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c1));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c2));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c3));

	mock().expectOneCall("connector_process").withParameter("self", &c1);
	mock().expectOneCall("connector_process").withParameter("self", &c2);
	mock().expectOneCall("connector_process").withParameter("self", &c3);

	LONGS_EQUAL(0, charger_process(&charger));
}
TEST(Charger, ShouldReturnConnectorID_WhenConnectorGiven) {
	struct connector c1;
	struct connector c2;
	struct connector c3;
	int id1, id2, id3;
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c1));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c2));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c3));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c1, &id1));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c2, &id2));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c3, &id3));
	LONGS_EQUAL(1, id1);
	LONGS_EQUAL(2, id2);
	LONGS_EQUAL(3, id3);
}
