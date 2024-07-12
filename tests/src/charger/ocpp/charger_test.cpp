#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "connector_private.h"
#include "csms.h"
#include "safety.h"
#include "iec61851.h"
#include "updater.h"

#include "libmcu/metrics.h"
#include <time.h>

time_t time(time_t *arg) {
	time_t t = (time_t)mock().actualCall(__func__).returnIntValue();
	if (arg) {
		*arg = t;
	}
	return t;
}

void ocpp_generate_message_id(void *buf, size_t bufsize) {
	snprintf((char *)buf, bufsize, "1234");
}

int ocpp_send(const struct ocpp_message *msg) {
	info("Sending message: %s, %s", ocpp_stringify_type(msg->type), msg->id);
	return 0;
}

int ocpp_recv(struct ocpp_message *msg) {
	return 0;
}

int csms_init(void *ctx) {
	return ocpp_init(NULL, ctx);
	//return 0;
}

int csms_request(const ocpp_message_t msgtype, void *ctx, void *opt) {
	return mock().actualCall(__func__)
		.withParameter("msgtype", msgtype).returnIntValue();
}

int updater_register_event_callback(updater_event_callback_t cb, void *cb_ctx) {
	return 0;
}

static struct connector *m_connector;

static void on_event(struct charger *charger, struct connector *connector,
		const charger_event_t event, void *ctx) {
	m_connector = connector;
	mock().actualCall(__func__).withIntParameter("event", event);
}

TEST_GROUP(OcppCharger) {
	struct charger *charger;
	int now;
	uint32_t next_period_ms;

	void setup(void) {
		metrics_init(true);
		now = 0;

		struct charger_param param = {
			.max_input_current_mA = 31818,
			.input_voltage = 220,
			.input_frequency = 60,
		};
		struct connector_param connector_param = {
			.max_output_current_mA = 31818,
			.min_output_current_mA = 31818,
			.iec61851 = (struct iec61851 *)0xdeadc0de,
			.metering = (struct metering *)0xdeadbeef,
			.name = "connector1",
			.priority = 1,
		};

		mock().expectNCalls(2, "time").andReturnValue(now);
		mock().expectOneCall("metering_enable").andReturnValue(0);
		charger = ocpp_charger_create(&param, 0);
		charger_create_connector(charger, &connector_param);
		charger_register_event_cb(charger, on_event, NULL);
		mock().checkExpectations();
		mock().clear();
	}
	void teardown(void) {
		mock().expectOneCall("metering_disable").andReturnValue(0);
		ocpp_charger_destroy(charger);

		mock().checkExpectations();
		mock().clear();
	}

	void go_state_a(void) {
		now += 2;
		mock().expectOneCall("time").andReturnValue(now); // ocpp_step
		mock().expectOneCall("time").andReturnValue(now); // is_initial
		mock().expectOneCall("pilot_duty").andReturnValue(0); // is_initial
		mock().expectOneCall("iec61851_set_current").withParameter("milliampere", 0); // do_boot
		mock().expectOneCall("time").andReturnValue(now); // ocpp_generate_message_id
		mock().expectOneCall("time").andReturnValue(now); // on_state_change
		mock().expectOneCall("iec61851_is_occupied").andReturnValue(false); // event callback
		charger_step(charger, &next_period_ms);
	}
};

TEST(OcppCharger, ShouldReturnBooting_WhenJustStarted) {
	struct ocpp_connector *c = (struct ocpp_connector *)get_connector_by_id(charger, 1);
	STRCMP_EQUAL("Booting", stringify_status(get_status(c)));
}
#if 0
TEST(OcppCharger, t) {
	mock().ignoreOtherCalls();
	mock().expectNCalls(3, "iec61851_state").andReturnValue(IEC61851_STATE_F);
	charger_step(charger, &next_period_ms);
	STRCMP_EQUAL("Faulted", charger_state(charger, "connector1"));
}
TEST(OcppCharger, ShouldDoNothing_WhenJustBooted) {
	mock().expectOneCall("time").andReturnValue(now); // ocpp_step
	mock().expectOneCall("time").andReturnValue(now); // is_initial
	mock().expectOneCall("pilot_duty").andReturnValue(0); // is_initial
	charger_step(charger, &next_period_ms);
}
TEST(OcppCharger, ShouldPushBootNotification_WhenBootedSuccessfully) {
	now += 2;
	mock().expectOneCall("time").andReturnValue(now); // ocpp_step
	mock().expectOneCall("time").andReturnValue(now); // is_initial
	mock().expectOneCall("pilot_duty").andReturnValue(0); // is_initial
	mock().expectOneCall("iec61851_set_current").withParameter("milliampere", 0); // do_boot
	mock().expectOneCall("time").andReturnValue(now); // ocpp_generate_message_id
	mock().expectOneCall("time").andReturnValue(now); // on_state_change
	mock().expectOneCall("iec61851_is_occupied").andReturnValue(false); // event callback
	charger_step(charger, &next_period_ms);
}
#endif
