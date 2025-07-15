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

#ifndef OCPP_CHARGER_H
#define OCPP_CHARGER_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include "charger/charger.h"
#include "charger/ocpp_checkpoint.h"

/* connector 0 is a special connector that represents the charger itself in
 * OCPP */
#define CONNECTOR_0				((void *)-1)

typedef enum {
	OCPP_CHARGER_EVENT_REBOOT_REQUIRED       = 0x01,
	OCPP_CHARGER_EVENT_CONFIGURATION_CHANGED = 0x02,
	OCPP_CHARGER_EVENT_CHECKPOINT_CHANGED    = 0x04,
} ocpp_charger_event_t;

typedef enum {
	OCPP_CHARGER_REBOOT_NONE,
	OCPP_CHARGER_REBOOT_REQUIRED, /* will wait for charging to be ended */
	OCPP_CHARGER_REBOOT_REQUIRED_REMOTELY, /* charging will be stopped */
	OCPP_CHARGER_REBOOT_FORCED, /* immediate reboot */
} ocpp_charger_reboot_t;

typedef enum {
	OCPP_CHARGER_MSG_AVAILABILITY_CHANGED, /* when connector 0 changed */
	OCPP_CHARGER_MSG_CONFIGURATION_CHANGED,
	OCPP_CHARGER_MSG_BILLING_STARTED,
	OCPP_CHARGER_MSG_BILLING_ENDED,
	OCPP_CHARGER_MSG_CSMS_UP,
	OCPP_CHARGER_MSG_REMOTE_RESET,
	OCPP_CHARGER_MSG_MAX,
} ocpp_charger_msg_t;

struct ocpp_charger_msg {
	ocpp_charger_msg_t type;
	void *value;
};

struct ocpp_charger;

/**
 * @brief Creates a new OCPP charger instance.
 *
 * @return Pointer to the newly created charger instance.
 */
struct charger *ocpp_charger_create(void *ctx);

/**
 * @brief Destroys an OCPP charger instance.
 *
 * @param[in] charger Pointer to the charger instance to be destroyed.
 */
void ocpp_charger_destroy(struct charger *charger);

/**
 * @brief Creates a new OCPP charger extension.
 *
 * @return Pointer to the newly created charger extension.
 */
struct charger_extension *ocpp_charger_extension(void);

/**
 * @brief Gets the pending reboot type for the given charger.
 *
 * @param[in] charger Pointer to the charger instance.
 *
 * @return The pending reboot type.
 */
ocpp_charger_reboot_t
ocpp_charger_get_pending_reboot_type(const struct charger *charger);

/**
 * @brief Requests a reboot for the given charger.
 *
 * @param[in] charger Pointer to the charger instance.
 * @param[in] type The type of reboot to request.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ocpp_charger_request_reboot(struct charger *charger,
		ocpp_charger_reboot_t type);

/**
 * @brief Retrieves the checkpoint for the OCPP charger.
 *
 * This function returns the current checkpoint for the specified OCPP charger.
 *
 * @param[in] charger A pointer to the charger.
 *
 * @return A pointer to the current checkpoint.
 */
struct ocpp_checkpoint *ocpp_charger_get_checkpoint(struct charger *charger);

/**
 * @brief Sets the checkpoint for the given charger.
 *
 * @param[in] charger Pointer to the charger instance.
 * @param[in] checkpoint Pointer to the checkpoint structure to be set.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ocpp_charger_set_checkpoint(struct charger *charger,
		const struct ocpp_checkpoint *checkpoint);

/**
 * @brief Checks if the checkpoint of the given charger is equal to the
 * specified checkpoint.
 *
 * @param[in] charger Pointer to the charger instance.
 * @param[in] checkpoint Pointer to the checkpoint to compare against.
 *
 * @return True if the checkpoints are equal, false otherwise.
 */
bool ocpp_charger_is_checkpoint_equal(const struct charger *charger,
		const struct ocpp_checkpoint *checkpoint);

/**
 * @brief Sends a message through the given charger.
 *
 * @param[in] charger Pointer to the charger instance.
 * @param[in] type The type of the message to be sent.
 * @param[in] value Pointer to the value associated with the message.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ocpp_charger_mq_send(struct charger *charger,
		ocpp_charger_msg_t type, void *value);

/**
 * @brief Receives a message for the given charger.
 *
 * @param[in] charger Pointer to the charger instance.
 * @param[in] msg Pointer to the message received.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ocpp_charger_mq_recv(struct charger *charger, struct ocpp_charger_msg *msg);

/**
 * @brief Gets the connector associated with the given transaction ID.
 *
 * @param[in] charger Pointer to the charger instance.
 * @param[in] transaction_id The transaction ID.
 *
 * @return Pointer to the connector associated with the given transaction ID, or
 *         NULL if not found.
 */
struct connector *ocpp_charger_get_connector_by_tid(struct charger *charger,
		const ocpp_transaction_id_t transaction_id);

/**
 * @brief Sets the availability of the given charger.
 *
 * @param[in] charger Pointer to the charger instance.
 * @param[in] available Boolean indicating whether the charger is available.
 */
void ocpp_charger_set_availability(struct charger *charger, bool available);

/**
 * @brief Gets the availability of the given charger.
 *
 * @param[in] charger Pointer to the charger instance.
 *
 * @return Boolean indicating whether the charger is available.
 */
bool ocpp_charger_get_availability(const struct charger *charger);

#if defined(__cplusplus)
}
#endif

#endif /* OCPP_CHARGER_H */
