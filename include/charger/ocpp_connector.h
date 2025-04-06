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

#ifndef OCPP_CONNECTOR_H
#define OCPP_CONNECTOR_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "charger/connector.h"
#include "charger/ocpp_checkpoint.h"

typedef enum {
	Booting,	/* 0 */
	Faulted,	/* 1 */
	Available,	/* 2 */
	Preparing,	/* 3 */
	Charging,	/* 4 */
	SuspendedEV,	/* 5 */
	SuspendedEVSE,	/* 6 */
	Finishing,	/* 7 */
	Reserved,	/* 8 */
	Unavailable,	/* 9 */
} ocpp_connector_state_t;

typedef enum {
	OCPP_SESSION_RESULT_OK,
	OCPP_SESSION_RESULT_REJECTED,
	OCPP_SESSION_RESULT_NOT_ALLOWED,
	OCPP_SESSION_RESULT_ALREADY_OCCUPIED,
	OCPP_SESSION_RESULT_NOT_MATCHED, /* no entry or release UID mismatch */
	OCPP_SESSION_RESULT_NOT_SUPPORTED,
	OCPP_SESSION_RESULT_TIMEOUT,
	OCPP_SESSION_RESULT_PENDING,
	OCPP_SESSION_RESULT_ERROR,
} ocpp_session_result_t;

typedef void (*ocpp_session_result_cb_t)(struct connector *c,
		ocpp_session_result_t result, void *ctx);

struct ocpp_connector;

/**
 * @brief Creates an OCPP connector.
 *
 * This function initializes and returns a new OCPP connector using the provided
 * parameters.
 *
 * @param[in] param A pointer to the connector parameters.
 *
 * @return A pointer to the newly created connector.
 */
struct connector *ocpp_connector_create(const struct connector_param *param);

/**
 * @brief Destroys an OCPP connector.
 *
 * This function cleans up and deallocates the resources associated with the
 * given OCPP connector.
 *
 * @param[in] c A pointer to the connector to be destroyed.
 */
void ocpp_connector_destroy(struct connector *c);

/**
 * @brief Links a checkpoint to the OCPP connector.
 *
 * This function associates a checkpoint with the given OCPP connector.
 *
 * @param[in] c A pointer to the connector.
 * @param[in] checkpoint A pointer to the checkpoint to be linked.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ocpp_connector_link_checkpoint(struct connector *c,
		struct ocpp_connector_checkpoint *checkpoint);

/**
 * @brief Checks if the OCPP connector has a specific transaction.
 *
 * This function checks if the given OCPP connector has a transaction with the
 * specified ID.
 *
 * @param[in] c A pointer to the connector.
 * @param[in] tid The transaction ID.
 *
 * @return A boolean indicating whether the transaction is present.
 */
bool ocpp_connector_has_transaction(struct connector *c,
		const ocpp_transaction_id_t tid);

/**
 * @brief Attempts to verify whether the connector can be occupied by a given
 * id.
 *
 * This function performs authorization checks (local list, authorization cache,
 * or CSMS via Authorize.req) and reports the result asynchronously via the
 * callback. It does not change the internal session or connector state.
 *
 * @param[in] c          The connector instance to be verified.
 * @param[in] id         Pointer to the idTag (or equivalent ID) to be verified.
 * @param[in] id_len     Length in bytes of the idTag.
 * @param[in] remote     Whether the request is initiated by a remote command
 *                       (e.g., via CSMS).
 * @param[in] cb         Callback function to be invoked with the result.
 * @param[in] cb_ctx     User-defined context to be passed to the callback.
 *
 * @return 0 on success of scheduling the check, negative errno otherwise.
 *
 * @note This function may return asynchronously after remote server
 *       interaction. To proceed with actual occupation, call
 *       `ocpp_connector_occupy()` inside the callback.
 */
int ocpp_connector_try_occupy(struct connector *c,
		const void *id, size_t id_len, bool remote,
		ocpp_session_result_cb_t cb, void *cb_ctx);

/**
 * @brief Occupies the connector for a session with the previously verified ID.
 *
 * This function finalizes the connector occupation, marking the beginning of a
 * new session. It must be called after a successful
 * `ocpp_connector_try_occupy()` operation.
 *
 * @param[in] c      The connector instance to be occupied.
 *
 * @return 0 on success, or negative errno on failure (e.g., no matching prior
 *         try_occupy call).
 */
int ocpp_connector_occupy(struct connector *c);

/**
 * @brief Attempts to verify whether the current session can be released with
 * the provided ID.
 *
 * This function checks whether the given ID is authorized to stop the current
 * charging session. It may perform authorization checks (e.g., idTag match,
 * parentIdTag match, CSMS check), and invokes the result callback with the
 * decision.
 *
 * @param[in] c          The connector instance from which the session may be
 *                       released.
 * @param[in] id         Pointer to the idTag (or equivalent ID) to be verified.
 * @param[in] id_len     Length in bytes of the idTag.
 * @param[in] cb         Callback function to be invoked with the verification
 *                       result.
 * @param[in] cb_ctx     User-defined context to be passed to the callback.
 *
 * @return 0 if the check is initiated successfully, or negative errno on
 *           failure.
 *
 * @note This function does not modify any connector state. It only performs
 *       verification. To actually release the session, call
 *       `ocpp_connector_release()` inside the callback.
 */
int ocpp_connector_try_release(struct connector *c,
		const void *id, size_t id_len,
		ocpp_session_result_cb_t cb, void *cb_ctx);

/**
 * @brief Finalizes the release of a session previously verified via `try_release`.
 *
 * This function stops the current charging session and clears the internal session state.
 * It must be called only after a successful result from `ocpp_connector_try_release()`.
 *
 * @param c      The connector instance whose session should be released.
 *
 * @return 0 on success, or negative errno on failure (e.g., no session, id mismatch).
 *
 * @note This function may send OCPP StopTransaction.req if appropriate.
 *       Ensure that session validation (e.g., id match) is already performed.
 */
int ocpp_connector_release(struct connector *c, bool remote);

bool ocpp_connector_has_active_authorization(struct connector *c);
bool ocpp_connector_can_user_release(struct connector *c);

#if defined(__cplusplus)
}
#endif

#endif /* OCPP_CONNECTOR_H */
