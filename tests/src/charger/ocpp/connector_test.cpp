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

#include "charger/ocpp_connector.h"
#include "ocpp/ocpp.h"
#include "ocpp_connector_internal.h"
#include "iec61851.h"
#include "safety.h"

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

	void go_available(void) {
		ocpp_connector_set_csms_up(oc, true);
		mock().expectNCalls(2, "iec61851_state")
			.andReturnValue(IEC61851_STATE_A);
		mock().expectOneCall("csms_request")
			.withParameter("msgtype", OCPP_MSG_STATUS_NOTIFICATION);
		// on_state_change()
		mock().expectOneCall("iec61851_state")
			.andReturnValue(IEC61851_STATE_A);
		mock().expectOneCall("iec61851_is_occupied_state")
			.withParameter("state", IEC61851_STATE_A)
			.andReturnValue(false);
		connector_process(c);
	}
	void go_preparing_occupied(void) {
		go_available();

		// 1. fault check(is_evse_error)
		mock().expectOneCall("iec61851_get_pwm_duty_target")
			.andReturnValue(100);
		mock().expectNCalls(2, "safety_check")
			.ignoreOtherParameters()
			.andReturnValue(0);
		mock().expectOneCall("iec61851_state")
			.andReturnValue(IEC61851_STATE_A);
		// 2. is_unavailable, is_preparing, do_preparing
		mock().expectNCalls(3, "iec61851_state")
			.andReturnValue(IEC61851_STATE_A);
		mock().expectOneCall("csms_request")
			.withParameter("msgtype", OCPP_MSG_STATUS_NOTIFICATION);
		// 3. on_state_change()
		mock().expectOneCall("iec61851_state")
			.andReturnValue(IEC61851_STATE_A);
		mock().expectOneCall("iec61851_is_occupied_state")
			.withParameter("state", IEC61851_STATE_A)
			.andReturnValue(false);
		mock().expectOneCall("on_connector_event")
			.withParameter("event", CONNECTOR_EVENT_OCCUPIED);

		ocpp_connector_set_session_uid(oc, "test");

		connector_process(c);
	}
	void go_start_transaction(void) {
		go_preparing_occupied();

		// 1. fault check(is_evse_error)
		mock().expectOneCall("iec61851_get_pwm_duty_target")
			.andReturnValue(100);
		mock().expectNCalls(2, "safety_check")
			.ignoreOtherParameters().andReturnValue(0);
		mock().expectOneCall("iec61851_state")
			.andReturnValue(IEC61851_STATE_B);
		// 2. is_unavailable, is_available
		mock().expectNCalls(2, "iec61851_state")
			.andReturnValue(IEC61851_STATE_B);
		// 3. is_charging_rdy
		mock().expectOneCall("iec61851_state")
			.andReturnValue(IEC61851_STATE_B);
		mock().expectOneCall("ocpp_count_pending_requests")
			.andReturnValue(0);
		// 4. do_request_tid
		mock().expectOneCall("csms_request")
			.withParameter("msgtype", OCPP_MSG_START_TRANSACTION);
		connector_process(c);
	}
	void go_charging(void) {
		go_start_transaction();

		ocpp_connector_set_session_uid(oc, "test");
		ocpp_connector_set_tid(oc, 1);

		// 1. fault check(is_evse_error)
		mock().expectOneCall("iec61851_get_pwm_duty_target")
			.andReturnValue(100);
		mock().expectNCalls(2, "safety_check")
			.ignoreOtherParameters().andReturnValue(0);
		mock().expectOneCall("iec61851_state")
			.andReturnValue(IEC61851_STATE_B);
		// 2. is_unavailable, is_available
		mock().expectNCalls(2, "iec61851_state").
			andReturnValue(IEC61851_STATE_B);
		// 3. is_charging_rdy
		mock().expectOneCall("iec61851_state")
			.andReturnValue(IEC61851_STATE_B);
		// 4. is_charging
		mock().expectOneCall("iec61851_state")
			.andReturnValue(IEC61851_STATE_B);
		// 5. do_charging
		mock().expectOneCall("iec61851_set_current")
			.withParameter("milliampere", 32);
		// 6. on_state_change()
		mock().expectOneCall("iec61851_state")
			.andReturnValue(IEC61851_STATE_C);
		mock().expectOneCall("iec61851_is_occupied_state")
			.withParameter("state", IEC61851_STATE_C)
			.andReturnValue(true);
		mock().expectOneCall("on_connector_event")
			.withParameter("event", CONNECTOR_EVENT_CHARGING_STARTED);

		connector_process(c);
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
	// on_state_change()
	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_A);
	mock().expectOneCall("iec61851_is_occupied_state")
		.withParameter("state", IEC61851_STATE_A).andReturnValue(false);

	connector_process(c);
}
TEST(OcppConnector, ShouldGoPreparing_WhenBootedWithPlugged) {
	ocpp_connector_set_csms_up(oc, true);
	mock().expectNCalls(3, "iec61851_state").andReturnValue(IEC61851_STATE_B);
	mock().expectOneCall("csms_request")
		.withParameter("msgtype", OCPP_MSG_STATUS_NOTIFICATION);
	// on_state_change()
	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_B);
	mock().expectOneCall("iec61851_is_occupied_state")
		.withParameter("state", IEC61851_STATE_B).andReturnValue(true);

	connector_process(c);
}
TEST(OcppConnector, ShouldGoUnavailable_WhenBooted) {
	ocpp_connector_set_csms_up(oc, true);
	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_A);
	mock().expectOneCall("csms_request")
		.withParameter("msgtype", OCPP_MSG_STATUS_NOTIFICATION);
	// on_state_change()
	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_A);
	mock().expectOneCall("iec61851_is_occupied_state")
		.withParameter("state", IEC61851_STATE_A).andReturnValue(false);

	ocpp_connector_set_availability(oc, false);
	connector_process(c);
}
TEST(OcppConnector, ShouldGoPreparing_WhenAuthorized) {
	go_preparing_occupied();
}
TEST(OcppConnector, ShouldDispatchUnoccupiedEvent_WhenAuthorizationTimeout) {
	go_preparing_occupied();
	ocpp_connector_set_session_current_expiry(oc, time(NULL));

	// 1. fault check(is_evse_error)
	mock().expectOneCall("iec61851_get_pwm_duty_target").andReturnValue(100);
	mock().expectNCalls(2, "safety_check").ignoreOtherParameters().andReturnValue(0);
	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_A);
	// 2. is_unavailable, is_available, do_preparing
	mock().expectNCalls(2, "iec61851_state").andReturnValue(IEC61851_STATE_A);
	mock().expectOneCall("csms_request")
		.withParameter("msgtype", OCPP_MSG_STATUS_NOTIFICATION);
	// 3. on_state_change()
	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_A);
	mock().expectOneCall("iec61851_is_occupied_state")
		.withParameter("state", IEC61851_STATE_A)
		.andReturnValue(false);
	mock().expectOneCall("on_connector_event")
		.withParameter("event", CONNECTOR_EVENT_UNOCCUPIED);

	connector_process(c);
}
TEST(OcppConnector, ShouldSendStartTransaction_WhenGotStateBAfterAuthorized) {
	go_start_transaction();
}
TEST(OcppConnector, ShouldGoCharging_WhenPluggedAfterAuthorizedFirst) {
	go_start_transaction();

	ocpp_connector_set_session_uid(oc, "test");
	ocpp_connector_set_tid(oc, 1);

	// 1. fault check(is_evse_error)
	mock().expectOneCall("iec61851_get_pwm_duty_target").andReturnValue(100);
	mock().expectNCalls(2, "safety_check").ignoreOtherParameters().andReturnValue(0);
	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_B);
	// 2. is_unavailable, is_available
	mock().expectNCalls(2, "iec61851_state").andReturnValue(IEC61851_STATE_B);
	// 3. is_charging_rdy
	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_B);
	// 4. is_charging
	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_B);
	// 5. do_charging
	mock().expectOneCall("iec61851_set_current").withParameter("milliampere", 32);
	// 6. on_state_change()
	mock().expectOneCall("iec61851_state").andReturnValue(IEC61851_STATE_C);
	mock().expectOneCall("iec61851_is_occupied_state")
		.withParameter("state", IEC61851_STATE_C)
		.andReturnValue(true);
	mock().expectOneCall("on_connector_event")
		.withParameter("event", CONNECTOR_EVENT_CHARGING_STARTED);

	connector_process(c);
}
