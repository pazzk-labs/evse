#include "CppUTestExt/MockSupport.h"
#include "metering.h"

int metering_get_voltage(struct metering *self, int32_t *millivolt) {
	return (int)mock().actualCall(__func__)
		.withOutputParameter("millivolt", millivolt)
		.returnIntValue();
}

int metering_get_current(struct metering *self, int32_t *milliamp) {
	return (int)mock().actualCall(__func__)
		.withOutputParameter("milliamp", milliamp)
		.returnIntValue();
}
int metering_get_power_factor(struct metering *self, int32_t *centi) {
	return (int)mock().actualCall(__func__)
		.withOutputParameter("centi", centi)
		.returnIntValue();
}

int metering_get_frequency(struct metering *self, int32_t *centihertz) {
	return (int)mock().actualCall(__func__)
		.withOutputParameter("centihertz", centihertz)
		.returnIntValue();
}

int metering_get_phase(struct metering *self, int32_t *centidegree, int hz) {
	return (int)mock().actualCall(__func__)
		.withOutputParameter("centidegree", centidegree)
		.withParameter("hz", hz)
		.returnIntValue();
}

int metering_get_energy(struct metering *self, uint64_t *wh, uint64_t *varh) {
	return (int)mock().actualCall(__func__)
		.withOutputParameter("wh", wh)
		.withOutputParameter("varh", varh)
		.returnIntValue();
}

int metering_get_power(struct metering *self, int32_t *watt, int32_t *var) {
	return (int)mock().actualCall(__func__)
		.withOutputParameter("watt", watt)
		.withOutputParameter("var", var)
		.returnIntValue();
}

int metering_step(struct metering *self) {
	return (int)mock().actualCall(__func__).returnIntValue();
}

int metering_enable(struct metering *self) {
	return (int)mock().actualCall(__func__).returnIntValue();
}

int metering_disable(struct metering *self) {
	return (int)mock().actualCall(__func__).returnIntValue();
}

struct metering *metering_create(const metering_t type,
		const struct metering_param *param,
		metering_save_cb_t save_cb, void *save_cb_ctx) {
	return (struct metering *)mock().actualCall(__func__).returnPointerValue();
}

void metering_destroy(struct metering *self) {
	mock().actualCall(__func__);
}
