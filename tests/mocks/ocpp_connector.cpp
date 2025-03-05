#include "CppUTestExt/MockSupport.h"
#include "charger/ocpp_connector.h"

struct connector *ocpp_connector_create(const struct connector_param *param) {
	return (struct connector *)mock().actualCall("ocpp_connector_create")
		.returnPointerValueOrDefault(NULL);
}

void ocpp_connector_destroy(struct connector *c) {
	mock().actualCall("ocpp_connector_destroy");
}

int ocpp_connector_link_checkpoint(struct connector *c,
		struct ocpp_connector_checkpoint *checkpoint) {
	return mock().actualCall("ocpp_connector_link_checkpoint")
		.returnIntValueOrDefault(0);
}

bool ocpp_connector_has_message(struct connector *c,
		const uint8_t *mid, size_t mid_len) {
	return mock().actualCall("ocpp_connector_has_message")
		.returnBoolValueOrDefault(false);
}

bool ocpp_connector_has_transaction(struct connector *c,
		const ocpp_transaction_id_t tid) {
	return mock().actualCall("ocpp_connector_has_transaction")
		.returnBoolValueOrDefault(false);
}
