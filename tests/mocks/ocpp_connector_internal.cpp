#include "CppUTestExt/MockSupport.h"
#include "ocpp_connector_internal.h"

ocpp_connector_state_t ocpp_connector_state(struct ocpp_connector *oc) {
	return mock().actualCall("ocpp_connector_state")
		.returnIntValueOrDefault(Booting);
}
const char *ocpp_connector_stringify_state(ocpp_connector_state_t state) {
	return mock().actualCall("ocpp_connector_stringify_state")
		.returnStringValueOrDefault("Booting");
}
connector_state_t
ocpp_connector_map_state_to_common(ocpp_connector_state_t state) {
	return (connector_state_t)mock().actualCall("ocpp_connector_map_state_to_common")
		.returnIntValueOrDefault(F);
}
ocpp_status_t ocpp_connector_map_state_to_ocpp(ocpp_connector_state_t state) {
	return (ocpp_status_t)mock().actualCall("ocpp_connector_map_state_to_ocpp")
		.returnIntValueOrDefault(OCPP_STATUS_FAULTED);
}
ocpp_error_t ocpp_connector_map_error_to_ocpp(connector_error_t error) {
	return (ocpp_error_t)mock().actualCall("ocpp_connector_map_error_to_ocpp")
		.returnIntValueOrDefault(OCPP_ERROR_OTHER);
}

bool ocpp_connector_is_occupied(const ocpp_connector_state_t state) {
	return mock().actualCall("ocpp_connector_is_occupied")
		.returnBoolValueOrDefault(false);
}
bool ocpp_connector_is_csms_up(const struct ocpp_connector *oc) {
	return mock().actualCall("ocpp_connector_is_csms_up")
		.returnBoolValueOrDefault(false);
}
bool ocpp_connector_is_session_established(const struct ocpp_connector *oc) {
	return mock().actualCall("ocpp_connector_is_session_established")
		.returnBoolValueOrDefault(false);
}
bool ocpp_connector_is_session_active(const struct ocpp_connector *oc) {
	return mock().actualCall("ocpp_connector_is_session_active")
		.returnBoolValueOrDefault(false);
}
bool ocpp_connector_is_session_trial_id_exist(const struct ocpp_connector *oc) {
	return mock().actualCall("ocpp_connector_is_session_trial_id_exist")
		.returnBoolValueOrDefault(false);
}
bool ocpp_connector_is_transaction_started(const struct ocpp_connector *oc) {
	return mock().actualCall("ocpp_connector_is_transaction_started")
		.returnBoolValueOrDefault(false);
}

bool ocpp_connector_is_unavailable(const struct ocpp_connector *oc) {
	return mock().actualCall("ocpp_connector_is_unavailable")
		.returnBoolValueOrDefault(false);
}
void ocpp_connector_set_availability(struct ocpp_connector *oc,
		const bool available) {
	mock().actualCall("ocpp_connector_set_availability");
}

void ocpp_connector_clear_session(struct ocpp_connector *oc) {
	mock().actualCall("ocpp_connector_clear_session");
}

ocpp_measurand_t ocpp_connector_update_metering(struct ocpp_connector *oc) {
	return (ocpp_measurand_t)mock().actualCall("ocpp_connector_update_metering")
		.returnIntValueOrDefault(OCPP_MEASURAND_ENERGY_ACTIVE_IMPORT_REGISTER);
}
ocpp_measurand_t
ocpp_connector_update_metering_clock_aligned(struct ocpp_connector *oc) {
	return (ocpp_measurand_t)mock().actualCall("ocpp_connector_update_metering_clock_aligned")
		.returnIntValueOrDefault(OCPP_MEASURAND_ENERGY_ACTIVE_IMPORT_REGISTER);
}
void ocpp_connector_set_csms_up(struct ocpp_connector *oc, const bool up) {
	mock().actualCall("ocpp_connector_set_csms_up");
}

void ocpp_connector_set_session_current_expiry(struct ocpp_connector *oc,
		const time_t expiry) {
	mock().actualCall("ocpp_connector_set_session_current_expiry");
}
void ocpp_connector_set_remote_reset_requested(struct ocpp_connector *oc,
		const bool requested, const bool forced) {
	mock().actualCall("ocpp_connector_set_remote_reset_requested");
}
bool ocpp_connector_is_remote_reset_requested(struct ocpp_connector *oc,
		bool *forced) {
	return mock().actualCall("ocpp_connector_is_remote_reset_requested")
		.returnBoolValueOrDefault(false);
}

ocpp_stop_reason_t ocpp_connector_get_stop_reason(struct ocpp_connector *oc,
		ocpp_connector_state_t state) {
	return (ocpp_stop_reason_t)mock().actualCall("ocpp_connector_get_stop_reason")
		.returnIntValueOrDefault(OCPP_STOP_REASON_OTHER);
}
void ocpp_connector_set_tid(struct ocpp_connector *oc,
		const ocpp_transaction_id_t transaction_id) {
	mock().actualCall("ocpp_connector_set_tid");
}
void ocpp_connector_raise_event(struct ocpp_connector *oc,
		const connector_event_t event) {
	mock().actualCall("ocpp_connector_raise_event");
}
int ocpp_connector_record_mid(struct ocpp_connector *oc, const char *mid) {
	return mock().actualCall("ocpp_connector_record_mid")
		.returnIntValueOrDefault(0);
}
