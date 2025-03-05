#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "charger/ocpp_connector.h"
#include "ocpp/ocpp.h"
#include "ocpp_connector_internal.h"
#include "iec61851.h"

static void on_connector_event(struct connector *self, connector_event_t event,
		void *ctx) {
	mock().actualCall("on_connector_event").withParameter("event", event);
}

TEST_GROUP(OcppConnector) {
	struct ocpp_connector *oc;
	struct connector *c;
	struct connector_param param;
	struct ocpp_connector_checkpoint checkpoint;

	void setup(void) {
		memset(&checkpoint, 0, sizeof(checkpoint));

		param = (struct connector_param) {
			.max_output_current_mA = 32,
			.input_frequency = 60,
			.iec61851 = (struct iec61851 *)-1,
			.name = "test",
		};
		c = ocpp_connector_create(&param);
		connector_register_event_cb(c, on_connector_event, NULL);
		mock().expectOneCall("on_connector_event")
			.withParameter("event", CONNECTOR_EVENT_ENABLED);
		connector_enable(c);

		ocpp_connector_link_checkpoint(c, &checkpoint);
		oc = (struct ocpp_connector *)c;
	}
	void teardown(void) {
		connector_disable(c);
		ocpp_connector_destroy(c);

		mock().checkExpectations();
		mock().clear();
	}
};

TEST(OcppConnector, ShouldSendBootNotification_WhenNoPendingRequests) {
	mock().expectOneCall("ocpp_count_pending_requests").andReturnValue(0);
	mock().expectOneCall("iec61851_set_current").withParameter("milliampere", 0);
	mock().expectOneCall("csms_request")
		.withParameter("msgtype", OCPP_MSG_BOOTNOTIFICATION);
	connector_process(c);
}
TEST(OcppConnector, ShouldNotSendBootNotification_WhenKeepBooting) {
	mock().expectOneCall("ocpp_count_pending_requests").andReturnValue(0);
	mock().expectOneCall("iec61851_set_current").withParameter("milliampere", 0);
	mock().expectOneCall("csms_request")
		.withParameter("msgtype", OCPP_MSG_BOOTNOTIFICATION);
	connector_process(c);

	mock().expectOneCall("ocpp_count_pending_requests").andReturnValue(1);
	connector_process(c);
}
TEST(OcppConnector, ShouldGoAvailable_WhenBooted) {
	ocpp_connector_set_csms_up(oc, true);
	mock().expectNCalls(2, "iec61851_state").andReturnValue(IEC61851_STATE_A);
	mock().expectOneCall("csms_request")
		.withParameter("msgtype", OCPP_MSG_STATUS_NOTIFICATION);

	connector_process(c);
}
TEST(OcppConnector, ShouldGoPreparing_WhenBootedWithPlugged) {
	ocpp_connector_set_csms_up(oc, true);
	mock().expectNCalls(4, "iec61851_state").andReturnValue(IEC61851_STATE_B);
	mock().expectOneCall("csms_request")
		.withParameter("msgtype", OCPP_MSG_STATUS_NOTIFICATION);

	connector_process(c);
}
TEST(OcppConnector, ShouldGoUnavailable_WhenBooted) {
	ocpp_connector_set_csms_up(oc, true);
	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_A);
	mock().expectOneCall("csms_request")
		.withParameter("msgtype", OCPP_MSG_STATUS_NOTIFICATION);

	ocpp_connector_set_availability(oc, false);
	connector_process(c);
}
