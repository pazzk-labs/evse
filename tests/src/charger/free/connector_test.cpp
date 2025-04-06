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

#include "charger/free_connector.h"
#include "connector_internal.h"
#include "iec61851.h"
#include "safety.h"
#include "metering.h"

#include <time.h>

struct safety_ctx {
	const char *name;
	safety_entry_status_t status;
	int freq;
};

static time_t fake_time;
static bool fake_time_expected;
static safety_entry_status_t fake_safety_status;

time_t time(time_t *t) {
	if (fake_time_expected) {
		return (time_t)mock().actualCall("time").returnLongIntValue();
	}
	return fake_time++;
}

int safety_check(struct safety *self, safety_error_callback_t cb, void *cb_ctx) {
	struct safety_ctx *p = (struct safety_ctx *)cb_ctx;
	p->status = fake_safety_status;
	return mock().actualCall("safety_check").returnIntValue();
}

static void on_connector_event(struct connector *self, connector_event_t event,
		void *ctx) {
	mock().actualCall("on_connector_event").withParameter("event", event);
}

TEST_GROUP(FreeConnector) {
	struct connector *c;
	struct connector_param param;

	void setup(void) {
		fake_time = 0;
		fake_time_expected = false;
		fake_safety_status = SAFETY_STATUS_OK;

		param = (struct connector_param) {
			.max_output_current_mA = 32,
			.input_frequency = 60,
			.iec61851 = (struct iec61851 *)-1,
			.name = "test",
		};
		c = free_connector_create(&param);
		connector_register_event_cb(c, on_connector_event, NULL);
		mock().expectOneCall("on_connector_event")
			.withParameter("event", CONNECTOR_EVENT_ENABLED);
		connector_enable(c);
	}
	void teardown(void) {
		connector_disable(c);
		free_connector_destroy(c);

		mock().checkExpectations();
		mock().clear();
	}

	void expect_time(time_t t) {
		fake_time_expected = true;
		mock().expectOneCall("time").andReturnValue(t);
	}

	void go_a(void) {
		mock().expectOneCall("iec61851_get_pwm_duty_target").andReturnValue(0);
		mock().expectOneCall("iec61851_get_pwm_duty").andReturnValue(0);
		mock().expectOneCall("iec61851_set_current")
			.withParameter("milliampere", 0);
		LONGS_EQUAL(0, connector_process(c));
	}
	void go_b(void) {
		go_a();
		mock().expectOneCall("iec61851_state")
			.andReturnValue(IEC61851_STATE_B);
		mock().expectOneCall("iec61851_set_current")
			.withParameter("milliampere", 32);
		mock().expectOneCall("on_connector_event")
			.withParameter("event", CONNECTOR_EVENT_PLUGGED);
		LONGS_EQUAL(0, connector_process(c));
	}
	void go_c(void) {
		go_b();
		mock().expectOneCall("iec61851_state")
			.andReturnValue(IEC61851_STATE_C);
		mock().expectOneCall("iec61851_get_pwm_duty_target")
			.andReturnValue(53);
		mock().expectOneCall("iec61851_state")
			.andReturnValue(IEC61851_STATE_C);
		mock().expectOneCall("iec61851_is_supplying_power")
			.andReturnValue(false);
		mock().expectOneCall("iec61851_start_power_supply");
		mock().expectOneCall("metering_get_energy")
			.ignoreOtherParameters()
			.andReturnValue(0);
		mock().expectOneCall("on_connector_event")
			.withParameter("event", CONNECTOR_EVENT_CHARGING_STARTED);
		LONGS_EQUAL(0, connector_process(c));
	}
	void go_f(void) {
		go_a();
		mock().expectNCalls(2, "iec61851_state")
			.andReturnValue(IEC61851_STATE_C);
		mock().expectOneCall("iec61851_set_state_f");
		mock().expectOneCall("iec61851_state")
			.andReturnValue(IEC61851_STATE_F);
		mock().expectOneCall("on_connector_event")
			.withParameter("event", CONNECTOR_EVENT_ERROR);
		LONGS_EQUAL(0, connector_process(c));
	}
};

TEST(FreeConnector, ShouldReturnNull_WhenNullParamGiven) {
	LONGS_EQUAL(NULL, free_connector_create(NULL));
}
TEST(FreeConnector, ShouldReturnNull_WhenParamIsInvalid) {
	param.iec61851 = NULL;
	LONGS_EQUAL(NULL, free_connector_create(&param));
}
TEST(FreeConnector, ShouldGoStateA_WhenJustBooted) {
	mock().expectOneCall("iec61851_get_pwm_duty_target").andReturnValue(0);
	mock().expectOneCall("iec61851_get_pwm_duty").andReturnValue(0);
	mock().expectOneCall("iec61851_set_current")
		.withParameter("milliampere", 0);
	LONGS_EQUAL(0, connector_process(c));
}
TEST(FreeConnector, ShouldGoStateBWithEvent_WhenPlugged) {
	go_a();

	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_B);
	mock().expectOneCall("iec61851_set_current")
		.withParameter("milliampere", 32);
	mock().expectOneCall("on_connector_event")
		.withParameter("event", CONNECTOR_EVENT_PLUGGED);

	LONGS_EQUAL(0, connector_process(c));
}
// going A to C directly is unexpected
TEST(FreeConnector, ShouldGoStateFWithEvent_WhenUnexpectedStateChangeInStateA) {
	go_a();

	mock().expectNCalls(2, "iec61851_state").andReturnValue(IEC61851_STATE_C);

	mock().expectOneCall("iec61851_set_state_f");
	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_F);

	mock().expectOneCall("on_connector_event")
		.withParameter("event", CONNECTOR_EVENT_ERROR);

	LONGS_EQUAL(0, connector_process(c));
}
TEST(FreeConnector, ShouldGoStateFWithEvent_WhenEVSEErrorInStateA) {
	go_a();

	mock().expectNCalls(4, "iec61851_state").andReturnValue(IEC61851_STATE_F);
	mock().expectOneCall("iec61851_get_pwm_duty_target").andReturnValue(0);

	mock().expectOneCall("iec61851_is_supplying_power").andReturnValue(false);
	mock().expectOneCall("iec61851_set_state_f");
	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_F);
	mock().expectOneCall("safety_check").ignoreOtherParameters().andReturnValue(0);

	mock().expectOneCall("on_connector_event")
		.withParameter("event", CONNECTOR_EVENT_ERROR);

	LONGS_EQUAL(0, connector_process(c));
}
TEST(FreeConnector, ShouldGoStateAWithEvent_WhenUnpluggedInStateB) {
	go_b();

	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_A);
	mock().expectOneCall("iec61851_set_current")
		.withParameter("milliampere", 0);
	mock().expectOneCall("on_connector_event")
		.withParameter("event", CONNECTOR_EVENT_UNPLUGGED);

	LONGS_EQUAL(0, connector_process(c));
}
TEST(FreeConnector, ShouldGoStateCWithEvent_WhenChargingStartedInStateB) {
	go_b();

	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_C);
	mock().expectOneCall("iec61851_get_pwm_duty_target").andReturnValue(53);
	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_C);
	mock().expectOneCall("iec61851_is_supplying_power").andReturnValue(false);
	mock().expectOneCall("iec61851_start_power_supply");
	mock().expectOneCall("metering_get_energy")
		.ignoreOtherParameters()
		.andReturnValue(0);
	mock().expectOneCall("on_connector_event")
		.withParameter("event", CONNECTOR_EVENT_CHARGING_STARTED);

	LONGS_EQUAL(0, connector_process(c));
}
TEST(FreeConnector, ShouldGoStateAWithEvent_WhenUnpluggedInStateC) {
	go_c();

	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_A);
	mock().expectOneCall("iec61851_is_supplying_power").andReturnValue(true);
	mock().expectOneCall("iec61851_stop_power_supply");
	mock().expectOneCall("iec61851_set_current")
		.withParameter("milliampere", 0);
	mock().expectOneCall("metering_get_energy")
		.ignoreOtherParameters()
		.andReturnValue(0);

	mock().expectOneCall("on_connector_event")
		.withParameter("event", CONNECTOR_EVENT_CHARGING_ENDED | CONNECTOR_EVENT_UNPLUGGED);

	LONGS_EQUAL(0, connector_process(c));
}
// Should keep duty for re-charging
TEST(FreeConnector, ShouldGoStateBWithEvent_WhenChargingEndedInStateC) {
	go_c();

	mock().expectNCalls(2, "iec61851_state").andReturnValue(IEC61851_STATE_B);
	mock().expectOneCall("iec61851_is_supplying_power").andReturnValue(true);
	mock().expectOneCall("iec61851_stop_power_supply");
	mock().expectOneCall("metering_get_energy")
		.ignoreOtherParameters()
		.andReturnValue(0);

	mock().expectOneCall("on_connector_event")
		.withParameter("event", CONNECTOR_EVENT_CHARGING_ENDED);

	LONGS_EQUAL(0, connector_process(c));
}
TEST(FreeConnector, ShouldDoNothing_WhenChangingToStateDInStateC) {
	go_c();

	mock().expectNCalls(3, "iec61851_state").andReturnValue(IEC61851_STATE_D);

	LONGS_EQUAL(0, connector_process(c));
}
TEST(FreeConnector, ShouldGoStateFWithEvent_WhenEVErrorInStateC) {
	go_c();

	mock().expectNCalls(4, "iec61851_state").andReturnValue(IEC61851_STATE_E);
	mock().expectOneCall("iec61851_is_supplying_power").andReturnValue(true);
	mock().expectOneCall("iec61851_stop_power_supply");
	mock().expectOneCall("iec61851_set_state_f");
	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_F);
	mock().expectOneCall("on_connector_event")
		.withParameter("event", CONNECTOR_EVENT_CHARGING_ENDED | CONNECTOR_EVENT_ERROR);

	LONGS_EQUAL(0, connector_process(c));
}
TEST(FreeConnector, ShouldGoStateFWithEvent_WhenEVSEErrorInStateC) {
	go_c();

	fake_safety_status = SAFETY_STATUS_STALE;
	mock().expectNCalls(4, "iec61851_state").andReturnValue(IEC61851_STATE_F);
	// connector_is_evse_error
	mock().expectOneCall("iec61851_get_pwm_duty_target").andReturnValue(53);
	mock().expectOneCall("safety_check").ignoreOtherParameters().andReturnValue(1);
	// do_evse_error
	mock().expectOneCall("iec61851_is_supplying_power").andReturnValue(true);
	mock().expectOneCall("iec61851_stop_power_supply");
	mock().expectOneCall("iec61851_set_state_f");
	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_F);
	mock().expectOneCall("safety_check").ignoreOtherParameters().andReturnValue(0);

	mock().expectOneCall("on_connector_event")
		.withParameter("event", CONNECTOR_EVENT_CHARGING_ENDED | CONNECTOR_EVENT_ERROR);

	LONGS_EQUAL(0, connector_process(c));
}
TEST(FreeConnector, ShouldGetBackToStateAWithEvent_WhenRecoveredFromEVSEError) {
	go_f();

	mock().expectNCalls(5, "iec61851_state").andReturnValue(IEC61851_STATE_F);
	// is_input_power_ok
	mock().expectOneCall("safety_check").ignoreOtherParameters().andReturnValue(0);
	// is_emergency_stop
	mock().expectOneCall("safety_check").ignoreOtherParameters().andReturnValue(0);
	
	mock().expectOneCall("iec61851_set_current")
		.withParameter("milliampere", 0);

	expect_time(fake_time + 6); // EV_RESPONSE_TIMEOUT
	expect_time(fake_time); // state change

	mock().expectOneCall("on_connector_event")
		.withParameter("event", CONNECTOR_EVENT_ERROR_RECOVERY);

	LONGS_EQUAL(0, connector_process(c));
}
TEST(FreeConnector, ShouldStayInStateF_WhenEVResponseTimeIsNotElapsed) {
	go_f();

	mock().expectNCalls(5, "iec61851_state").andReturnValue(IEC61851_STATE_F);
	// is_input_power_ok
	mock().expectOneCall("safety_check").ignoreOtherParameters().andReturnValue(0);
	// is_emergency_stop
	mock().expectOneCall("safety_check").ignoreOtherParameters().andReturnValue(0);
	
	LONGS_EQUAL(0, connector_process(c));
}
