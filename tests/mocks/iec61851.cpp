#include "CppUTestExt/MockSupport.h"
#include "iec61851.h"

iec61851_state_t iec61851_state(struct iec61851 *self) {
	return (iec61851_state_t)mock().actualCall(__func__).returnIntValue();
}

void iec61851_start_pwm(struct iec61851 *self, const uint8_t pct) {
	mock().actualCall(__func__).withParameter("pct", pct);
}

void iec61851_set_current(struct iec61851 *self, const uint32_t milliampere) {
	mock().actualCall(__func__).withParameter("milliampere", milliampere);
}

void iec61851_stop_pwm(struct iec61851 *self) {
	mock().actualCall(__func__);
}

void iec61851_set_state_f(struct iec61851 *self) {
	mock().actualCall(__func__);
}

uint8_t iec61851_get_pwm_duty(struct iec61851 *self) {
	return (uint8_t)mock().actualCall(__func__).returnUnsignedIntValue();
}

uint8_t iec61851_get_pwm_duty_set(struct iec61851 *self) {
	return (uint8_t)mock().actualCall(__func__).returnUnsignedIntValue();
}

void iec61851_start_power_supply(struct iec61851 *self) {
	mock().actualCall(__func__);
}

void iec61851_stop_power_supply(struct iec61851 *self) {
	mock().actualCall(__func__);
}

bool iec61851_is_supplying_power(struct iec61851 *self) {
	return mock().actualCall(__func__).returnBoolValue();
}

bool iec61851_is_charging(struct iec61851 *self) {
	return mock().actualCall(__func__).returnBoolValue();
}

bool iec61851_is_charging_state(const iec61851_state_t state) {
	return mock().actualCall(__func__)
		.withParameter("state", state)
		.returnBoolValue();
}

bool iec61851_is_occupied(struct iec61851 *self) {
	return mock().actualCall(__func__).returnBoolValue();
}

bool iec61851_is_occupied_state(const iec61851_state_t state) {
	return mock().actualCall(__func__)
		.withParameter("state", state)
		.returnBoolValue();
}

bool iec61851_is_error(struct iec61851 *self) {
	return mock().actualCall(__func__).returnBoolValue();
}

bool iec61851_is_error_state(const iec61851_state_t state) {
	return mock().actualCall(__func__)
		.withParameter("state", state)
		.returnBoolValue();
}

const char *iec61851_stringify_state(const iec61851_state_t state) {
	return mock().actualCall(__func__)
		.withParameter("state", state)
		.returnStringValue();
}

uint32_t iec61851_duty_to_milliampere(const uint8_t duty) {
	return mock().actualCall(__func__)
		.withParameter("duty", duty)
		.returnUnsignedIntValue();
}

uint8_t iec61851_milliampere_to_duty(const uint32_t milliampere) {
	return (uint8_t)mock().actualCall(__func__)
		.withParameter("milliampere", milliampere)
		.returnUnsignedIntValue();
}
