#include "CppUTestExt/MockSupport.h"
#include "connector_internal.h"

uint8_t connector_get_target_duty(const struct connector *self) {
	return (uint8_t)mock().actualCall("connector_get_target_duty")
		.returnIntValueOrDefault(0);
}

uint8_t connector_get_actual_duty(const struct connector *self) {
	return (uint8_t)mock().actualCall("connector_get_actual_duty")
		.returnIntValueOrDefault(0);
}

void connector_start_duty(struct connector *self) {
	mock().actualCall("connector_start_duty");
}

void connector_stop_duty(struct connector *self) {
	mock().actualCall("connector_stop_duty");
}

void connector_go_fault(struct connector *self) {
	mock().actualCall("connector_go_fault");
}

void connector_enable_power_supply(struct connector *self) {
	mock().actualCall("connector_enable_power_supply");
}

void connector_disable_power_supply(struct connector *self) {
	mock().actualCall("connector_disable_power_supply");
}

bool connector_is_supplying_power(const struct connector *self) {
	return mock().actualCall("connector_is_supplying_power")
		.returnBoolValueOrDefault(false);
}

bool connector_is_state_x(const struct connector *self,
		connector_state_t state) {
	return mock().actualCall("connector_is_state_x")
		.withParameter("state", state)
		.returnBoolValueOrDefault(false);
}

bool connector_is_state_x2(const struct connector *self,
		connector_state_t state) {
	return mock().actualCall("connector_is_state_x2")
		.withParameter("state", state)
		.returnBoolValueOrDefault(false);
}

bool connector_is_evse_error(struct connector *self, connector_state_t state) {
	return mock().actualCall("connector_is_evse_error")
		.withParameter("state", state)
		.returnBoolValueOrDefault(false);
}

bool connector_is_emergency_stop(const struct connector *self) {
	return mock().actualCall("connector_is_emergency_stop")
		.returnBoolValueOrDefault(false);
}

bool connector_is_input_power_ok(struct connector *self) {
	return mock().actualCall("connector_is_input_power_ok")
		.returnBoolValueOrDefault(false);
}

bool connector_is_output_power_ok(struct connector *self) {
	return mock().actualCall("connector_is_output_power_ok")
		.returnBoolValueOrDefault(false);
}

bool connector_is_ev_response_timeout(const struct connector *self,
		uint32_t elapsed_sec) {
	return mock().actualCall("connector_is_ev_response_timeout")
		.withParameter("elapsed_sec", elapsed_sec)
		.returnBoolValueOrDefault(false);
}

bool connector_set_reserved(struct connector *self) {
	return mock().actualCall("connector_set_reserved")
		.returnBoolValueOrDefault(false);
}

bool connector_clear_reserved(struct connector *self) {
	return mock().actualCall("connector_clear_reserved")
		.returnBoolValueOrDefault(false);
}

bool connector_validate_param(const struct connector_param *param) {
	return mock().actualCall("connector_validate_param")
		.returnBoolValueOrDefault(false);
}

const char *connector_stringify_state(const connector_state_t state) {
	return mock().actualCall("connector_stringify_state")
		.returnStringValueOrDefault("E");
}
