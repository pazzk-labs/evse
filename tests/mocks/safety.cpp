#include "CppUTestExt/MockSupport.h"
#include "safety.h"

int safety_init(struct gpio *input_power, struct gpio *output_power) {
	return mock().actualCall(__func__).returnIntValue();
}

void safety_deinit(void) {
	mock().actualCall(__func__);
}

int safety_enable(void) {
	return mock().actualCall(__func__).returnIntValue();
}

int safety_disable(void) {
	return mock().actualCall(__func__).returnIntValue();
}

safety_status_t safety_status(safety_t type, const uint8_t expected_freq) {
	return (safety_status_t)mock().actualCall(__func__)
		.withParameter("type", type)
		.withParameter("expected_freq", expected_freq)
		.returnIntValue();
}

uint8_t safety_get_frequency(safety_t type) {
	return (uint8_t)mock().actualCall(__func__)
		.withParameter("type", type)
		.returnIntValue();
}
