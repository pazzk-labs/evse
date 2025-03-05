#include "metering.h"

struct metering *metering_create(const metering_t type,
		const struct metering_param *param,
		metering_save_cb_t save_cb, void *save_cb_ctx) {
	return NULL;
}

void metering_destroy(struct metering *self) {
}

int metering_enable(struct metering *self) {
	return 0;
}

int metering_disable(struct metering *self) {
	return 0;
}

int metering_save_energy(struct metering *self) {
	return 0;
}

int metering_step(struct metering *self) {
	return 0;
}

int metering_get_voltage(struct metering *self, int32_t *millivolt) {
	return 0;
}

int metering_get_current(struct metering *self, int32_t *milliamp) {
	return 0;
}

int metering_get_power_factor(struct metering *self, int32_t *centi) {
	return 0;
}

int metering_get_frequency(struct metering *self, int32_t *centihertz) {
	return 0;
}

int metering_get_phase(struct metering *self, int32_t *centidegree, int hz) {
	return 0;
}

int metering_get_energy(struct metering *self, uint64_t *wh, uint64_t *varh) {
	return 0;
}

int metering_get_power(struct metering *self, int32_t *watt, int32_t *var) {
	return 0;
}
