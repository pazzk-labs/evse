/*
 * This file is part of the Pazzk project <https://pazzk.net/>.
 * Copyright (c) 2024 Pazzk <team@pazzk.net>.
 *
 * Community Version License (GPLv3):
 * This software is open-source and licensed under the GNU General Public
 * License v3.0 (GPLv3). You are free to use, modify, and distribute this code
 * under the terms of the GPLv3. For more details, see
 * <https://www.gnu.org/licenses/gpl-3.0.en.html>.
 * Note: If you modify and distribute this software, you must make your
 * modifications publicly available under the same license (GPLv3), including
 * the source code.
 *
 * Commercial Version License:
 * For commercial use, including redistribution or integration into proprietary
 * systems, you must obtain a commercial license. This license includes
 * additional benefits such as dedicated support and feature customization.
 * Contact us for more details.
 *
 * Contact Information:
 * Maintainer: 권경환 Kyunghwan Kwon (on behalf of the Pazzk Team)
 * Email: k@pazzk.net
 * Website: <https://pazzk.net/>
 *
 * Disclaimer:
 * This software is provided "as-is", without any express or implied warranty,
 * including, but not limited to, the implied warranties of merchantability or
 * fitness for a particular purpose. In no event shall the authors or
 * maintainers be held liable for any damages, whether direct, indirect,
 * incidental, special, or consequential, arising from the use of this software.
 */

#ifndef OCPP_CONNECTOR_INTERNAL_H
#define OCPP_CONNECTOR_INTERNAL_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "charger/ocpp_connector.h"
#include "../connector_internal.h"
#include "ocpp/ocpp.h"
#include "libmcu/msgq.h"

struct uid_store;

struct auth {
	uint8_t uid[OCPP_ID_TOKEN_MAXLEN];
	uint8_t pid[OCPP_ID_TOKEN_MAXLEN];
};

struct session_metering {
	time_t timestamp;
	time_t time_sample_periodic;
	time_t time_clock_periodic;

	uint64_t start_wh;
	uint64_t stop_wh;
	uint64_t wh;
	int32_t millivolt;
	int32_t milliamp;
	int32_t pf_centi;
	int32_t centi_hertz;
	int32_t centi_degree;
	int32_t watt;

	ocpp_reading_context_t context;
};

struct charging_session {
	ocpp_transaction_id_t transaction_id;
	uint32_t reservation_id;
	struct session_metering metering;

	struct {
		struct auth current;
		struct auth trial; /* use id field only, used for authorization
			requests like authorize or stopTransaction. If
			the authorization request is accepted, it is updated
			from trial to current. */
		time_t timestamp;
	} auth;

	struct {
		time_t started;
		time_t suspended;
		time_t expiry;
	} timestamp;

	bool remote_stop;
};

struct charger_ocpp_info {
	struct {
		char id[OCPP_CiString50];
		char *data;
		size_t datasize;
	} datatransfer;

	/* below fields are used for additional information. it must be freed
	 * after use. */
	const char *vendor_error_code; /* CiString50 */
};

struct ocpp_connector {
	struct connector base;
	/* checkpoint is used for power loss recovery. During initialization,
	 * it must be linked to the appropriate data for the connector using
	 * ocpp_connector_link_checkpoint(). */
	struct ocpp_connector_checkpoint *checkpoint;
	struct msgq *evtq;
	struct charging_session session;
	struct charger_ocpp_info ocpp_info;

	struct {
		ocpp_session_result_cb_t cb;
		void *cb_ctx;
	} auth;

	time_t now;

	bool csms_up; /* set when BootNotification.conf: Accepted */
	bool remote_reset_requested; /* set when soft reset requested */
	bool remote_reset_forced; /* set when hard reset requested */
};

/**
 * @brief Retrieves the state of the OCPP connector.
 *
 * This function returns the current state of the specified OCPP connector.
 *
 * @param[in] oc A pointer to the OCPP connector.
 *
 * @return The current state of the OCPP connector.
 */
ocpp_connector_state_t ocpp_connector_state(struct ocpp_connector *oc);

/**
 * @brief Converts the OCPP connector state to a string.
 *
 * This function returns a string representation of the specified OCPP connector
 * state.
 *
 * @param[in] state The state to be converted to a string.
 *
 * @return A string representation of the state.
 */
const char *ocpp_connector_stringify_state(ocpp_connector_state_t state);

/**
 * @brief Maps the given connector state to a common connector state.
 *
 * @param[in] state The connector state to be mapped.
 *
 * @return The corresponding common connector state.
 */
connector_state_t
ocpp_connector_map_state_to_common(ocpp_connector_state_t state);

/**
 * @brief Maps the given connector state to an OCPP status.
 *
 * @param[in] state The connector state to be mapped.
 *
 * @return The corresponding OCPP status.
 */
ocpp_status_t ocpp_connector_map_state_to_ocpp(ocpp_connector_state_t state);

/**
 * @brief Maps the given connector error to an OCPP error.
 *
 * @param[in] error The connector error to be mapped.
 *
 * @return The corresponding OCPP error.
 */
ocpp_error_t ocpp_connector_map_error_to_ocpp(connector_error_t error);

/**
 * @brief Checks if the CSMS is up for the OCPP connector.
 *
 * This function checks if the Central System Management System (CSMS) is up for
 * the specified OCPP connector.
 *
 * @param[in] oc A pointer to the OCPP connector.
 *
 * @return A boolean indicating whether the CSMS is up.
 */
bool ocpp_connector_is_csms_up(const struct ocpp_connector *oc);

/**
 * @brief Sets the CSMS status for the OCPP connector.
 *
 * This function sets the status of the Central System Management System (CSMS)
 * for the specified OCPP connector.
 *
 * @param[in] oc A pointer to the OCPP connector.
 * @param[in] up A boolean indicating whether the CSMS is up.
 */
void ocpp_connector_set_csms_up(struct ocpp_connector *oc, const bool up);

/**
 * @brief Checks if the OCPP connector is unavailable.
 *
 * This function checks if the specified OCPP connector is unavailable.
 *
 * @param[in] oc A pointer to the OCPP connector.
 * @return A boolean indicating whether the connector is unavailable.
 */
bool ocpp_connector_is_unavailable(const struct ocpp_connector *oc);

/**
 * @brief Sets the availability of the OCPP connector.
 *
 * This function sets the availability status of the specified OCPP connector.
 *
 * @param[in] oc A pointer to the OCPP connector.
 * @param[in] available A boolean indicating the availability status to be set.
 */
void ocpp_connector_set_availability(struct ocpp_connector *oc,
		const bool available);

/**
 * @brief Updates the metering information for the OCPP connector.
 *
 * This function updates and returns the metering information for the specified
 * OCPP connector.
 *
 * @param[in] oc A pointer to the OCPP connector.
 *
 * @return The updated metering information.
 */
ocpp_measurand_t ocpp_connector_update_metering(struct ocpp_connector *oc);

/**
 * @brief Updates the clock-aligned metering information for the OCPP connector.
 *
 * This function updates and returns the clock-aligned metering information for
 * the specified OCPP connector.
 *
 * @param[in] oc A pointer to the OCPP connector.
 *
 * @return The updated clock-aligned metering information.
 */
ocpp_measurand_t
ocpp_connector_update_metering_clock_aligned(struct ocpp_connector *oc);

/**
 * @brief Sets the remote reset request status for the OCPP connector.
 *
 * @param[in] oc Pointer to the OCPP connector structure.
 * @param[in] requested Boolean indicating if a remote reset has been requested.
 * @param[in] forced Boolean indicating if the reset request is forced.
 */
void ocpp_connector_set_remote_reset_requested(struct ocpp_connector *oc,
		const bool requested, const bool forced);

/**
 * @brief Checks if a remote reset has been requested for the OCPP connector.
 *
 * @param[in] oc Pointer to the OCPP connector structure.
 * @param[out] forced Pointer to a boolean that will be set to true if the reset
 *             request is forced.
 *
 * @return true if a remote reset has been requested, false otherwise.
 */
bool ocpp_connector_is_remote_reset_requested(struct ocpp_connector *oc,
		bool *forced);

/**
 * @brief Gets the stop reason for the OCPP connector.
 *
 * @param[in] oc Pointer to the OCPP connector structure.
 * @param[in] state The state of the OCPP connector.
 *
 * @return The stop reason.
 */
ocpp_stop_reason_t ocpp_connector_get_stop_reason(struct ocpp_connector *oc,
		ocpp_connector_state_t state);

/**
 * @brief Sets the transaction ID for the current charging session.
 *
 * @param[in] oc Pointer to the OCPP connector structure.
 * @param[in] transaction_id The transaction ID to set.
 */
void ocpp_connector_set_tid(struct ocpp_connector *oc,
		const ocpp_transaction_id_t transaction_id);

/**
 * @brief Clears the checkpoint transaction ID for the OCPP connector.
 *
 * The transaction ID is stored in persistent storage to prevent missing
 * transactions during system reboots, such as power loss, and this function
 * deletes it once the charging is completed.
 *
 * @param[in] oc Pointer to the OCPP connector structure.
 */
void ocpp_connector_clear_checkpoint_tid(struct ocpp_connector *oc);

/**
 * @brief Sets the current transaction ID as the checkpoint for the specified
 *        OCPP connector.
 *
 * This function assigns the current transaction ID to the given OCPP connector
 * as a checkpoint, which can be used to track the progress or state of a
 * transaction. It is used to recover transactions in unexpected system reboot
 * situations such as power loss.
 *
 * @param oc Pointer to the OCPP connector structure.
 */
void ocpp_connector_set_checkpoint_tid(struct ocpp_connector *oc);

/**
 * @brief Checks if the OCPP connector has a missing transaction.
 *
 * @note A missing transaction occurs when the checkpoint transaction ID exists.
 * @note Transaction ID 0 cannot be used. 0 indicates that there is no
 *       transaction.
 *
 * @param[in] oc Pointer to the OCPP connector structure.
 *
 * @return true if there is a missing transaction, false otherwise.
 */
bool ocpp_connector_has_missing_transaction(struct ocpp_connector *oc);

/**
 * @brief Raises an event for the OCPP connector.
 *
 * @param[in] oc Pointer to the OCPP connector structure.
 * @param[in] event The event to raise.
 */
void ocpp_connector_raise_event(struct ocpp_connector *oc,
		const connector_event_t event);

/**
 * @brief Checks if a charging session is established for a given user on the
 * specified OCPP connector.
 *
 * This function returns true if a charging session is currently established for
 * a given user on the specified OCPP connector, otherwise it returns false.
 *
 * @param oc Pointer to the OCPP connector structure.
 * @return true if a session is established, false otherwise.
 */
bool ocpp_connector_is_session_established(const struct ocpp_connector *oc);

/**
 * @brief Checks if a charging session is active for the given OCPP connector.
 *
 * This function returns true if a charging session is currently active for the
 * specified OCPP connector, otherwise it returns false.
 *
 * @note If the user credentials have expired, this function will return false.
 *
 * @param oc Pointer to the OCPP connector structure.
 *
 * @return true if a session is active, false otherwise.
 */
bool ocpp_connector_is_session_active(const struct ocpp_connector *oc);

/**
 * @brief Checks if there is a pending session for the given connector.
 *
 * This function returns true if a session trial ID is currently associated with
 * the specified OCPP connector, otherwise it returns false.
 *
 * @note A trial ID is generated when the user makes an authentication request,
 *       such as card tagging. If the authentication request is successful,
 *       the trial ID is converted to a charging session ID.
 *
 * @param[in] oc Pointer to the OCPP connector structure.
 *
 * @return true if a session is pending, false otherwise.
 */
bool ocpp_connector_is_session_pending(const struct ocpp_connector *oc);

/**
 * @brief Checks if a transaction has started for the given OCPP connector.
 *
 * This function returns true if a transaction is currently started for the
 * specified OCPP connector, otherwise it returns false.
 *
 * @note Even if a charging session has started, charging may not have begun.
 *       A transaction indicates that charging is in progress, while a charging
 *       session includes preparatory steps such as user authentication.
 *
 * @param oc Pointer to the OCPP connector structure.
 *
 * @return true if a transaction is started, false otherwise.
 */
bool ocpp_connector_is_transaction_started(const struct ocpp_connector *oc);

/**
 * @brief Clears the current session for the given OCPP connector.
 *
 * This function clears any existing session associated with the specified OCPP
 * connector. It is typically used to reset the state of the connector after a
 * session has ended, cancelled or when an error occurs.
 *
 * @param oc Pointer to the OCPP connector structure.
 */
void ocpp_connector_clear_session(struct ocpp_connector *oc);

void ocpp_connector_set_session_uid(struct ocpp_connector *oc,
		const char uid[OCPP_ID_TOKEN_MAXLEN]);

/**
 * @brief Clears the session ID for the OCPP connector.
 *
 * @param[in] auth Pointer to the auth structure.
 */
void ocpp_connector_clear_session_id(struct auth *auth);

/**
 * @brief Sets the current session expiry time for the OCPP connector.
 *
 * @param[in] oc Pointer to the OCPP connector structure.
 * @param[in] expiry The expiry time to set.
 */
void ocpp_connector_set_session_current_expiry(struct ocpp_connector *oc,
		const time_t expiry);

/**
 * @brief Sets the parent session ID for the OCPP connector.
 *
 * @param[in] c Pointer to the OCPP connector structure.
 * @param[in] pid The parent session ID to set.
 */
void ocpp_connector_set_session_pid(struct ocpp_connector *c,
		const char pid[OCPP_ID_TOKEN_MAXLEN]);

/**
 * @brief Sets a session as pending for the given connector.
 *
 * This function marks a session as pending for the specified OCPP connector
 * and associates it with a unique identifier.
 *
 * @param[in] c Pointer to the OCPP connector structure.
 * @param[in] uid Unique identifier for the session, with a maximum length
 *            defined by OCPP_ID_TOKEN_MAXLEN.
 */
void ocpp_connector_set_session_pending(struct ocpp_connector *c,
		const char uid[OCPP_ID_TOKEN_MAXLEN]);

/**
 * @brief Clears the pending session state for the given connector.
 *
 * This function clears the trial UID associated with the specified OCPP
 * connector.
 *
 * @note It doesn't mean that no session is established. It only clears the
 * trial UID.
 *
 * @param[in] c Pointer to the OCPP connector structure.
 */
void ocpp_connector_clear_session_pending(struct ocpp_connector *c);

void ocpp_connector_set_auth_result_cb(struct ocpp_connector *oc,
		ocpp_session_result_cb_t cb, void *cb_ctx);
void ocpp_connector_clear_auth_result_cb(struct ocpp_connector *oc);
void ocpp_connector_dispatch_auth_result(struct ocpp_connector *oc,
		ocpp_session_result_t result);
bool ocpp_connector_is_session_uid_equal(struct connector *c,
		const char uid[OCPP_ID_TOKEN_MAXLEN]);
bool ocpp_connector_is_session_pid_equal(struct connector *c,
		const char pid[OCPP_ID_TOKEN_MAXLEN]);

#if defined(__cplusplus)
}
#endif

#endif /* OCPP_CONNECTOR_INTERNAL_H */
