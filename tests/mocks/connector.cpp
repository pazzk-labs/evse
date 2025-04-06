#include "CppUTestExt/MockSupport.h"
#include "charger/connector.h"

int connector_enable(struct connector *self) {
	return mock().actualCall("connector_enable")
		.returnIntValueOrDefault(0);
}

int connector_disable(struct connector *self) {
	return mock().actualCall("connector_disable")
		.returnIntValueOrDefault(0);
}

int connector_process(struct connector *self) {
	return mock().actualCall("connector_process")
		.withParameter("self", self)
		.returnIntValueOrDefault(0);
}

int connector_register_event_cb(struct connector *self,
		connector_event_cb_t cb, void *cb_ctx) {
	return mock().actualCall("connector_register_event_cb")
		.returnIntValueOrDefault(0);
}

connector_state_t connector_state(const struct connector *self) {
	return (connector_state_t)mock().actualCall("connector_state")
		.returnIntValueOrDefault(F);
}

const char *connector_name(const struct connector *self) {
	return mock().actualCall("connector_name")
		.returnStringValueOrDefault("test");
}

bool connector_is_enabled(const struct connector *self) {
	return mock().actualCall("connector_is_enabled")
		.returnBoolValueOrDefault(false);
}

bool connector_is_occupied(const struct connector *self) {
	return mock().actualCall("connector_is_occupied")
		.returnBoolValueOrDefault(false);
}
