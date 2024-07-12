#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "charger/free.h"
#include "connector_private.h"

#include "libmcu/metrics.h"
#include <time.h>

time_t time(time_t *arg) {
	time_t t = (time_t)mock().actualCall(__func__).returnIntValue();
	if (arg) {
		*arg = t;
	}
	return t;
}

static struct connector *m_connector;

static void on_event(struct charger *charger, struct connector *connector,
		const charger_event_t event, void *ctx) {
	m_connector = connector;
	mock().actualCall(__func__).withIntParameter("event", event);
}

TEST_GROUP(FreeCharger) {
	struct charger *charger;
	int now;

	void setup(void) {
		metrics_init(true);

		struct charger_param param = {
			.max_input_current_mA = 31818,
			.input_voltage = 220,
			.input_frequency = 60,
		};

		struct connector_param connector_param = {
			.max_output_current_mA = 31818,
			.min_output_current_mA = 31818,
			.iec61851 = (struct iec61851 *)0xdeadc0de,
			.name = "connector1",
			.priority = 1,
		};

		now = 0;
		mock().expectOneCall("time").andReturnValue(now);
		charger = free_charger_create(&param);
		charger_create_connector(charger, &connector_param);
		charger_register_event_cb(charger, on_event, NULL);
	}
	void teardown(void) {
		free_charger_destroy(charger);

		mock().checkExpectations();
		mock().clear();
	}

	void go_state_a(void) {
		now = 3;
		mock().expectOneCall("get_pwm_duty_set").andReturnValue(0);
		mock().expectOneCall("read_pwm_duty").andReturnValue(0);
		mock().expectOneCall("stop_pwm");

		mock().expectOneCall("time").andReturnValue(now);
		mock().expectOneCall("stringify_state").withParameter("state", E).andReturnValue("E");
		mock().expectOneCall("stringify_state").withParameter("state", A).andReturnValue("A");
		mock().expectOneCall("get_event_from_state_change")
			.withParameter("new_state", A)
			.withParameter("old_state", E)
			.andReturnValue(CHARGER_EVENT_NONE);

		charger_step(charger, NULL);
	}
	void go_state_a_f(void) {
		go_state_a();

		mock().expectNCalls(4, "is_state_x").ignoreOtherParameters().andReturnValue(false);
		mock().expectOneCall("is_evse_error").andReturnValue(true);

		expect_error(A, F);
		mock().expectOneCall("is_emergency_stop").andReturnValue(false);
		mock().expectOneCall("is_input_power_ok").andReturnValue(false);
		mock().expectOneCall("log_power_failure");

		expect_event(CHARGER_EVENT_ERROR, A, F);
		charger_step(charger, NULL);
	}
	void go_state_b(void) {
		go_state_a();

		mock().expectOneCall("is_state_x").withParameter("state", B).andReturnValue(true);
		mock().expectOneCall("start_pwm");

		mock().expectOneCall("time").andReturnValue(now);
		mock().expectOneCall("stringify_state").withParameter("state", A).andReturnValue("A");
		mock().expectOneCall("stringify_state").withParameter("state", B).andReturnValue("B");
		mock().expectOneCall("get_event_from_state_change")
			.withParameter("new_state", B)
			.withParameter("old_state", A)
			.andReturnValue(CHARGER_EVENT_PLUGGED);

		mock().expectOneCall("on_event").withIntParameter("event", CHARGER_EVENT_PLUGGED);

		charger_step(charger, NULL);
	}
	void go_state_c(void) {
		go_state_b();

		mock().expectOneCall("is_state_x").withParameter("state", A).andReturnValue(false);
		mock().expectOneCall("is_state_x2").withParameter("state", C).andReturnValue(true);
		mock().expectOneCall("is_supplying_power").andReturnValue(false);
		mock().expectOneCall("enable_power_supply");

		mock().expectOneCall("time").andReturnValue(now);
		mock().expectOneCall("stringify_state").withParameter("state", B).andReturnValue("B");
		mock().expectOneCall("stringify_state").withParameter("state", C).andReturnValue("C");
		mock().expectOneCall("get_event_from_state_change")
			.withParameter("new_state", C)
			.withParameter("old_state", B)
			.andReturnValue(CHARGER_EVENT_CHARGING_STARTED);

		mock().expectOneCall("on_event").withIntParameter("event", CHARGER_EVENT_CHARGING_STARTED);

		charger_step(charger, NULL);
	}
	void expect_event(charger_event_t event, connector_state_t from, connector_state_t to) {
		const char *tbl[] = { "A", "B", "C", "D", "E", "F", };
		mock().expectOneCall("time").andReturnValue(now);
		mock().expectOneCall("stringify_state").withParameter("state", to).andReturnValue(tbl[to]);
		mock().expectOneCall("stringify_state").withParameter("state", from).andReturnValue(tbl[from]);
		mock().expectOneCall("get_event_from_state_change")
			.withParameter("new_state", to)
			.withParameter("old_state", from)
			.andReturnValue(event);

		mock().expectOneCall("on_event").withIntParameter("event", event);
	}
	void expect_error(connector_state_t from, connector_state_t to) {
		const char *tbl[] = { "A", "B", "C", "D", "E", "F", };
		if (from == C || from == D) {
			mock().expectOneCall("is_supplying_power").andReturnValue(true);
			mock().expectOneCall("disable_power_supply");
		} else {
			mock().expectOneCall("is_supplying_power").andReturnValue(false);
		}
		mock().expectOneCall("go_fault");
		mock().expectOneCall("stringify_state").withParameter("state", from).andReturnValue(tbl[from]);
		mock().expectOneCall("get_state").andReturnValue(to);
		mock().expectOneCall("stringify_state").withParameter("state", to).andReturnValue(tbl[to]);
	}
};

IGNORE_TEST(FreeCharger, ShouldWaitForInputPowerStable_WhenChargerBegins) {
	mock().expectOneCall("get_pwm_duty_set").andReturnValue(0);
	mock().expectOneCall("read_pwm_duty").andReturnValue(0);

	charger_step(charger, NULL);
}
TEST(FreeCharger, ShouldSetCPTo100_WhenInputPowerStable) {
	mock().expectOneCall("get_pwm_duty_set").andReturnValue(0);
	mock().expectOneCall("read_pwm_duty").andReturnValue(0);
	mock().expectOneCall("stop_pwm");

	mock().expectOneCall("time").andReturnValue(3);
	mock().expectOneCall("stringify_state").withParameter("state", E).andReturnValue("E");
	mock().expectOneCall("stringify_state").withParameter("state", A).andReturnValue("A");
	mock().expectOneCall("get_event_from_state_change")
		.withParameter("new_state", A)
		.withParameter("old_state", E)
		.andReturnValue(CHARGER_EVENT_NONE);

	charger_step(charger, NULL);
}
TEST(FreeCharger, ShouldGoStateF_WhenInputPowerAbnormalFrequencyDetected) {
	go_state_a();

	mock().expectNCalls(4, "is_state_x").ignoreOtherParameters().andReturnValue(false);
	mock().expectOneCall("is_evse_error").andReturnValue(true);

	expect_error(A, F);
	mock().expectOneCall("is_emergency_stop").andReturnValue(false);
	mock().expectOneCall("is_input_power_ok").andReturnValue(false);
	mock().expectOneCall("log_power_failure");

	expect_event(CHARGER_EVENT_ERROR, A, F);

	charger_step(charger, NULL);
	LONGS_EQUAL(CONNECTOR_ERROR_EVSE_SIDE, connector_error(m_connector));
}
TEST(FreeCharger, ShouldStayStateFForAtLeastEVResponseTime) {
	go_state_a_f();

	mock().expectNCalls(5, "is_state_x").ignoreOtherParameters().andReturnValue(false);

	mock().expectOneCall("is_input_power_ok").andReturnValue(true);
	mock().expectOneCall("is_emergency_stop").andReturnValue(false);
	mock().expectOneCall("time").andReturnValue(now);
	mock().expectOneCall("is_early_recovery").andReturnValue(true);

	charger_step(charger, NULL);
}
TEST(FreeCharger, ShouldStayStateF_WhenInputPowerSamplingError) {
	go_state_a_f();

	mock().expectNCalls(5, "is_state_x").ignoreOtherParameters().andReturnValue(false);
	mock().expectOneCall("is_input_power_ok").andReturnValue(false);
	charger_step(charger, NULL);
	LONGS_EQUAL(CONNECTOR_ERROR_EVSE_SIDE, connector_error(m_connector));
}
TEST(FreeCharger, ShouldGoStateA_WhenEVSEErrorRecovered) {
	go_state_a_f();

	mock().expectNCalls(5, "is_state_x").ignoreOtherParameters().andReturnValue(false);

	mock().expectOneCall("is_input_power_ok").andReturnValue(true);
	mock().expectOneCall("is_emergency_stop").andReturnValue(false);
	mock().expectOneCall("time").andReturnValue(now);
	mock().expectOneCall("is_early_recovery").andReturnValue(false);

	mock().expectOneCall("stop_pwm");

	expect_event(CHARGER_EVENT_ERROR_RECOVERY, F, A);

	charger_step(charger, NULL);
}
TEST(FreeCharger, ShouldGoStateF_WhenUnexpectedCPStateDetectedInStateA) {
	go_state_a();

	mock().expectOneCall("is_state_x").withParameter("state", B).andReturnValue(false);
	mock().expectOneCall("is_state_x").withParameter("state", C).andReturnValue(true);

	mock().expectOneCall("go_fault");
	mock().expectOneCall("stringify_state").withParameter("state", A).andReturnValue("A");
	mock().expectOneCall("get_state").andReturnValue(F);
	mock().expectOneCall("stringify_state").withParameter("state", F).andReturnValue("F");

	expect_event(CHARGER_EVENT_ERROR, A, F);

	charger_step(charger, NULL);
	LONGS_EQUAL(CONNECTOR_ERROR_EVSE_SIDE, connector_error(m_connector));
}
TEST(FreeCharger, ShouldGoStateBSettingPWM_WhenPlugged) {
	go_state_a();

	mock().expectOneCall("is_state_x").withParameter("state", B).andReturnValue(true);
	mock().expectOneCall("start_pwm");

	expect_event(CHARGER_EVENT_PLUGGED, A, B);

	charger_step(charger, NULL);
}
TEST(FreeCharger, ShouldGoStateA_WhenUnplugged) {
	go_state_b();

	mock().expectOneCall("is_state_x").withParameter("state", A).andReturnValue(true);
	mock().expectOneCall("stop_pwm");

	expect_event(CHARGER_EVENT_UNPLUGGED, B, A);

	charger_step(charger, NULL);
}
TEST(FreeCharger, ShouldGoStateC_WhenEVReadyToCharge) {
	go_state_b();

	mock().expectOneCall("is_state_x").withParameter("state", A).andReturnValue(false);
	mock().expectOneCall("is_state_x2").withParameter("state", C).andReturnValue(true);
	mock().expectOneCall("is_supplying_power").andReturnValue(false);
	mock().expectOneCall("enable_power_supply");

	expect_event(CHARGER_EVENT_CHARGING_STARTED, B, C);

	charger_step(charger, NULL);
}
TEST(FreeCharger, ShouldGoStateF_WhenEVErrorDetected) { // diode fault
	go_state_b();

	mock().expectOneCall("is_state_x").ignoreOtherParameters().andReturnValue(false);
	mock().expectNCalls(2, "is_state_x2").ignoreOtherParameters().andReturnValue(false);
	mock().expectOneCall("is_state_x").withParameter("state", E).andReturnValue(true);

	expect_error(B, F);
	expect_event(CHARGER_EVENT_ERROR, B, F);

	charger_step(charger, NULL);
	LONGS_EQUAL(CONNECTOR_ERROR_EV_SIDE, connector_error(m_connector));
}
TEST(FreeCharger, ShouldGoStateA_WhenUnpluggedInStateC) {
	go_state_c();

	mock().expectOneCall("is_state_x").withParameter("state", A).andReturnValue(true);
	mock().expectOneCall("is_supplying_power").andReturnValue(true);
	mock().expectOneCall("disable_power_supply");
	mock().expectOneCall("stop_pwm");

	expect_event((charger_event_t)(CHARGER_EVENT_UNPLUGGED | CHARGER_EVENT_CHARGING_ENDED), C, A);

	charger_step(charger, NULL);
}
TEST(FreeCharger, ShouldGoStateB_WhenEVSuspended) {
	go_state_c();

	mock().expectOneCall("is_state_x").withParameter("state", A).andReturnValue(false);
	mock().expectOneCall("is_state_x").withParameter("state", B).andReturnValue(true);
	mock().expectOneCall("is_supplying_power").andReturnValue(true);
	mock().expectOneCall("disable_power_supply");

	expect_event(CHARGER_EVENT_CHARGING_SUSPENDED, C, B);

	charger_step(charger, NULL);
}
TEST(FreeCharger, ShouldGoStateFAfterCleanup_WhenEVErrorDetected) {
	go_state_c();

	mock().expectNCalls(3, "is_state_x").ignoreOtherParameters().andReturnValue(false);
	mock().expectOneCall("is_state_x").withParameter("state", E).andReturnValue(true);

	expect_error(C, F);
	expect_event(CHARGER_EVENT_ERROR, C, F);

	charger_step(charger, NULL);
	LONGS_EQUAL(CONNECTOR_ERROR_EV_SIDE, connector_error(m_connector));
}
TEST(FreeCharger, ShouldGoStateFAfterCleanup_WhenEVSEErrorDetected) {
	go_state_c();

	mock().expectNCalls(4, "is_state_x").ignoreOtherParameters().andReturnValue(false);
	mock().expectOneCall("is_evse_error").andReturnValue(true);

	expect_error(C, F);
	mock().expectOneCall("is_emergency_stop").andReturnValue(false);
	mock().expectOneCall("is_input_power_ok").andReturnValue(false);
	mock().expectOneCall("log_power_failure");

	expect_event((charger_event_t)(CHARGER_EVENT_UNPLUGGED | CHARGER_EVENT_CHARGING_ENDED), C, F);

	charger_step(charger, NULL);
}
IGNORE_TEST(FreeCharger, ShouldGoStateF_WhenOutputPowerErrorDetectedInStateC) {
	go_state_c();

	mock().expectNCalls(4, "is_state_x").ignoreOtherParameters().andReturnValue(false);
	mock().expectOneCall("is_evse_error").andReturnValue(true);

	expect_error(C, F);
	mock().expectOneCall("is_emergency_stop").andReturnValue(false);
	mock().expectOneCall("is_input_power_ok").andReturnValue(true);
	mock().expectOneCall("is_output_power_ok").andReturnValue(false);
	mock().expectOneCall("log_power_failure");

	expect_event((charger_event_t)(CHARGER_EVENT_UNPLUGGED | CHARGER_EVENT_CHARGING_ENDED), C, F);

	charger_step(charger, NULL);
	LONGS_EQUAL(CONNECTOR_ERROR_EVSE_SIDE, connector_error(m_connector));
}
IGNORE_TEST(FreeCharger, ShouldStayStateCEvenWhenOutputPowerErrorDetectedInStateC_WhenInitialStablizationDelayNotElapsed) {
	go_state_c();

	mock().expectNCalls(4, "is_state_x").ignoreOtherParameters().andReturnValue(false);
	mock().expectOneCall("get_pwm_duty_set").andReturnValue(53);
	mock().expectOneCall("is_input_power_ok").andReturnValue(true);
	mock().expectOneCall("is_emergency_stop").andReturnValue(false);
	mock().expectOneCall("is_output_power_ok").andReturnValue(false);
	mock().expectOneCall("time").andReturnValue(now);

	charger_step(charger, NULL);
}
IGNORE_TEST(FreeCharger, ShouldGoStateF_WhenEmergencyStopDetectedInStateC) {
	go_state_c();

	mock().expectNCalls(4, "is_state_x").ignoreOtherParameters().andReturnValue(false);
	mock().expectOneCall("get_pwm_duty_set").andReturnValue(53);
	mock().expectOneCall("is_input_power_ok").andReturnValue(true);
	mock().expectOneCall("is_emergency_stop").andReturnValue(true);

	expect_error(C, F);
	mock().expectOneCall("is_emergency_stop").andReturnValue(true);
	mock().expectOneCall("is_input_power_ok").andReturnValue(true);

	expect_event((charger_event_t)(CHARGER_EVENT_ERROR | CHARGER_EVENT_CHARGING_ENDED), C, F);

	charger_step(charger, NULL);
	LONGS_EQUAL(CONNECTOR_ERROR_EMERGENCY_STOP, connector_error(m_connector));
}
TEST(FreeCharger, ShouldGoStateB_WhenRecoveryWhileOccupied) {
	go_state_a_f();

	mock().expectNCalls(5, "is_state_x").ignoreOtherParameters().andReturnValue(false);
	mock().expectOneCall("is_input_power_ok").andReturnValue(true);
	mock().expectOneCall("is_emergency_stop").andReturnValue(false);
	mock().expectOneCall("time").andReturnValue(now+6);
	mock().expectOneCall("is_early_recovery").andReturnValue(false);

	mock().expectOneCall("stop_pwm");

	expect_event(CHARGER_EVENT_ERROR_RECOVERY, F, A);

	charger_step(charger, NULL);

	mock().expectOneCall("is_state_x").withParameter("state", B).andReturnValue(true);
	mock().expectOneCall("start_pwm");
	expect_event(CHARGER_EVENT_PLUGGED, A, B);

	charger_step(charger, NULL);
}
