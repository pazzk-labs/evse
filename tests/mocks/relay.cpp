#include "CppUTestExt/MockSupport.h"
#include "relay.h"

struct relay *relay_create(struct pwm_channel *pwm_handle) {
	return (struct relay *)mock().actualCall(__func__).returnPointerValue();
}

void relay_destroy(struct relay *self) {
	mock().actualCall(__func__);
}

int relay_enable(struct relay *self) {
	return mock().actualCall(__func__).returnIntValue();
}

int relay_disable(struct relay *self) {
	return mock().actualCall(__func__).returnIntValue();
}

void relay_turn_on(struct relay *self) {
	mock().actualCall(__func__);
}

void relay_turn_off(struct relay *self) {
	mock().actualCall(__func__);
}

void relay_turn_on_ext(struct relay *self,
        uint8_t pickup_pct, uint16_t pickup_delay_ms, uint8_t hold_pct) {
	mock().actualCall(__func__);
}
