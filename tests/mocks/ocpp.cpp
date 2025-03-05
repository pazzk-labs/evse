#include "CppUTestExt/MockSupport.h"
#include "ocpp/ocpp.h"

int ocpp_init(ocpp_event_callback_t cb, void *cb_ctx) {
	return mock().actualCall("ocpp_init")
		.returnIntValueOrDefault(0);
}
int ocpp_step(void) {
	return mock().actualCall("ocpp_step")
		.returnIntValueOrDefault(0);
}
int ocpp_push_request(ocpp_message_t type, const void *data, size_t datasize,
		struct ocpp_message **created) {
	return mock().actualCall("ocpp_push_request")
		.withParameter("type", type)
		.returnIntValueOrDefault(0);
}
int ocpp_push_request_force(ocpp_message_t type, const void *data,
		size_t datasize, struct ocpp_message **created) {
	return mock().actualCall("ocpp_push_request_force")
		.withParameter("type", type)
		.returnIntValueOrDefault(0);
}
int ocpp_push_request_defer(ocpp_message_t type,
		const void *data, size_t datasize, uint32_t timer_sec) {
	return mock().actualCall("ocpp_push_request_defer")
		.withParameter("type", type)
		.returnIntValueOrDefault(0);
}
int ocpp_push_response(const struct ocpp_message *req,
		const void *data, size_t datasize, bool err) {
	return mock().actualCall("ocpp_push_response")
		.returnIntValueOrDefault(0);
}
size_t ocpp_count_pending_requests(void) {
	return mock().actualCall("ocpp_count_pending_requests")
		.returnUnsignedIntValueOrDefault(0);
}
const char *ocpp_stringify_type(ocpp_message_t msgtype) {
	return mock().actualCall("ocpp_stringify_type")
		.returnStringValueOrDefault("BootNotification");
}
ocpp_message_t ocpp_get_type_from_string(const char *typestr) {
	return (ocpp_message_t)mock().actualCall("ocpp_get_type_from_string")
		.returnIntValueOrDefault(OCPP_MSG_BOOTNOTIFICATION);
}
ocpp_message_t ocpp_get_type_from_idstr(const char *idstr) {
	return (ocpp_message_t)mock().actualCall("ocpp_get_type_from_idstr")
		.returnIntValueOrDefault(OCPP_MSG_BOOTNOTIFICATION);
}

bool ocpp_has_configuration(const char * const keystr) {
	return mock().actualCall("ocpp_has_configuration")
		.returnBoolValueOrDefault(false);
}
size_t ocpp_count_configurations(void) {
	return mock().actualCall("ocpp_count_configurations")
		.returnUnsignedIntValueOrDefault(0);
}
size_t ocpp_compute_configuration_size(void) {
	return mock().actualCall("ocpp_compute_configuration_size")
		.returnUnsignedIntValueOrDefault(0);
}
int ocpp_copy_configuration_from(const void *data, size_t datasize) {
	return mock().actualCall("ocpp_copy_configuration_from")
		.returnIntValueOrDefault(0);
}
int ocpp_copy_configuration_to(void *buf, size_t bufsize) {
	return mock().actualCall("ocpp_copy_configuration_to")
		.returnIntValueOrDefault(0);
}
void ocpp_reset_configuration(void) {
	mock().actualCall("ocpp_reset_configuration");
}
int ocpp_set_configuration(const char * const keystr,
		const void *value, size_t value_size) {
	return mock().actualCall("ocpp_set_configuration")
		.returnIntValueOrDefault(0);
}
int ocpp_get_configuration(const char * const keystr,
		void *buf, size_t bufsize, bool *readonly) {
	return mock().actualCall("ocpp_get_configuration")
		.returnIntValueOrDefault(0);
}
int ocpp_get_configuration_by_index(int index,
		void *buf, size_t bufsize, bool *readonly) {
	return mock().actualCall("ocpp_get_configuration_by_index")
		.returnIntValueOrDefault(0);
}
bool ocpp_is_configuration_writable(const char * const keystr) {
	return mock().actualCall("ocpp_is_configuration_writable")
		.returnBoolValueOrDefault(false);
}
bool ocpp_is_configuration_readable(const char * const keystr) {
	return mock().actualCall("ocpp_is_configuration_readable")
		.returnBoolValueOrDefault(false);
}
size_t ocpp_get_configuration_size(const char * const keystr) {
	return mock().actualCall("ocpp_get_configuration_size")
		.returnUnsignedIntValueOrDefault(0);
}
ocpp_configuration_data_t ocpp_get_configuration_data_type(
		const char * const keystr) {
	return (ocpp_configuration_data_t)mock().actualCall("ocpp_get_configuration_data_type")
		.returnIntValueOrDefault(OCPP_CONF_TYPE_STR);
}
const char *ocpp_get_configuration_keystr_from_index(int index) {
	return mock().actualCall("ocpp_get_configuration_keystr_from_index")
		.returnStringValueOrDefault("key");
}
const char *ocpp_stringify_configuration_value(const char *keystr,
		char *buf, size_t bufsize) {
	return mock().actualCall("ocpp_stringify_configuration_value")
		.returnStringValueOrDefault("value");
}
