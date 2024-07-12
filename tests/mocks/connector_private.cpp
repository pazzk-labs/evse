#include "CppUTestExt/MockSupport.h"
#include "connector_private.h"

connector_state_t get_state(const struct connector *c)
{
	return (connector_state_t)mock().actualCall("get_state")
		.returnIntValueOrDefault(F);
}

uint8_t get_pwm_duty_set(const struct connector *c)
{
	return (uint8_t)mock().actualCall("get_pwm_duty_set")
		.returnIntValueOrDefault(0);
}

uint8_t read_pwm_duty(const struct connector *c)
{
	return (uint8_t)mock().actualCall("read_pwm_duty")
		.returnIntValueOrDefault(0);
}

void start_pwm(struct connector *c)
{
	mock().actualCall("start_pwm");
}

void stop_pwm(struct connector *c)
{
	mock().actualCall("stop_pwm");
}

void go_fault(struct connector *c)
{
	mock().actualCall("go_fault");
}

void enable_power_supply(struct connector *c)
{
	mock().actualCall("enable_power_supply");
}

void disable_power_supply(struct connector *c)
{
	mock().actualCall("disable_power_supply");
}

bool is_state_x(const struct connector *c, connector_state_t state)
{
	return mock().actualCall("is_state_x").withParameter("state", state)
		.returnBoolValueOrDefault(false);
}

bool is_state_x2(const struct connector *c, connector_state_t state)
{
	return mock().actualCall("is_state_x2").withParameter("state", state)
		.returnBoolValueOrDefault(false);
}

bool is_evse_error(const struct connector *c, connector_state_t state)
{
	return mock().actualCall("is_evse_error").returnBoolValueOrDefault(false);
}

bool is_supplying_power(const struct connector *c)
{
	return mock().actualCall("is_supplying_power")
		.returnBoolValueOrDefault(false);
}

bool is_emergency_stop(const struct connector *c)
{
	return mock().actualCall("is_emergency_stop")
		.returnBoolValueOrDefault(false);
}

bool is_input_power_ok(const struct connector *c)
{
	return mock().actualCall("is_input_power_ok")
		.returnBoolValueOrDefault(false);
}

bool is_output_power_ok(const struct connector *c)
{
	return mock().actualCall("is_output_power_ok")
		.returnBoolValueOrDefault(false);
}

bool is_early_recovery(uint32_t elapsed_sec)
{
	return mock().actualCall("is_early_recovery")
		.returnBoolValueOrDefault(false);
}

charger_event_t get_event_from_state_change(const connector_state_t new_state,
		const connector_state_t old_state)
{
	return (charger_event_t)mock().actualCall("get_event_from_state_change")
		.withParameter("new_state", new_state)
		.withParameter("old_state", old_state)
		.returnIntValueOrDefault(CHARGER_EVENT_NONE);
}

const char *stringify_state(const connector_state_t state)
{
	return (const char *)mock().actualCall("stringify_state")
		.withParameter("state", state)
		.returnStringValueOrDefault("unknown");
}

void log_power_failure(const struct connector *c)
{
	mock().actualCall("log_power_failure");
}
