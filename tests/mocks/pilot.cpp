#include "CppUTestExt/MockSupport.h"
#include "pilot.h"

struct pilot *pilot_create(const struct pilot_params *params,
        struct adc122s051 *adc, struct lm_pwm_channel *pwm, uint16_t *buf) {
	return (struct pilot *)mock().actualCall(__func__).returnPointerValue();
}

void pilot_delete(struct pilot *pilot) {
	mock().actualCall(__func__);
}

void pilot_set_status_cb(struct pilot *pilot,
		pilot_status_cb_t cb, void *cb_ctx) {
	mock().actualCall(__func__);
}

int pilot_enable(struct pilot *pilot) {
	return mock().actualCall(__func__).returnIntValue();
}

int pilot_disable(struct pilot *pilot) {
	return mock().actualCall(__func__).returnIntValue();
}

int pilot_set_duty(struct pilot *pilot, const uint8_t pct) {
	return mock().actualCall(__func__)
		.withParameter("pct", pct).returnIntValue();
}

uint8_t pilot_get_duty_set(const struct pilot *pilot) {
	return (uint8_t)mock().actualCall(__func__).returnUnsignedIntValue();
}

uint8_t pilot_duty(const struct pilot *pilot) {
	return (uint8_t)mock().actualCall(__func__).returnUnsignedIntValue();
}

pilot_status_t pilot_status(const struct pilot *pilot) {
	return (pilot_status_t)mock().actualCall(__func__).returnIntValue();
}

uint16_t pilot_millivolt(const struct pilot *pilot, const bool low_voltage) {
	return (uint16_t)mock().actualCall(__func__).returnUnsignedIntValue();
}

bool pilot_ok(const struct pilot *pilot) {
	return mock().actualCall(__func__).returnBoolValue();
}

pilot_error_t pilot_error(const struct pilot *pilot) {
	return (pilot_error_t)mock().actualCall(__func__).returnUnsignedIntValue();
}

const char *pilot_stringify_status(const pilot_status_t status) {
	return mock().actualCall(__func__).returnStringValue();
}

void pilot_default_params(struct pilot_params *params) {
	mock().actualCall(__func__);
}
