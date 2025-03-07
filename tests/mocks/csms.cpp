#include "CppUTestExt/MockSupport.h"
#include "csms.h"

int csms_init(void *ctx) {
	return mock().actualCall("csms_init")
		.returnIntValueOrDefault(0);
}
int csms_request(const ocpp_message_t msgtype, void *ctx, void *opt) {
	return mock().actualCall("csms_request")
		.withParameter("msgtype", msgtype)
		.returnIntValueOrDefault(0);
}
int csms_request_defer(const ocpp_message_t msgtype, void *ctx, void *opt,
		const uint32_t delay_sec) {
	return mock().actualCall("csms_request_defer")
		.withParameter("msgtype", msgtype)
		.withParameter("delay_sec", delay_sec)
		.returnIntValueOrDefault(0);
}
int csms_response(const ocpp_message_t msgtype,
		const struct ocpp_message *req, void *ctx, void *opt) {
	return mock().actualCall("csms_response")
		.withParameter("msgtype", msgtype)
		.returnIntValueOrDefault(0);
}
bool csms_is_up(void) {
	return mock().actualCall("csms_is_up")
		.returnBoolValueOrDefault(true);
}
int csms_reconnect(const uint32_t delay_sec) {
	return mock().actualCall("csms_reconnect")
		.withParameter("delay_sec", delay_sec)
		.returnIntValueOrDefault(0);
}
