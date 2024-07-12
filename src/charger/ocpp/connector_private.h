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

#ifndef OCPP_CONNECTOR_PRIVATE_H
#define OCPP_CONNECTOR_PRIVATE_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "../connector_private.h"
#include "charger_private.h"
#include "libmcu/fsm.h"
#include "ocpp/ocpp.h"

#define CONNECTOR_0		((void *)-1)

enum connector_status {
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
};
typedef int16_t ocpp_connector_status_t;

struct auth {
	uint8_t id[OCPP_ID_TOKEN_MAXLEN];
	uint8_t parent_id[OCPP_ID_TOKEN_MAXLEN];
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
	ocpp_charger_transaction_id_t transaction_id;
	uint32_t reservation_id;
	struct session_metering metering;

	struct {
		struct auth current;
		struct auth trial; /* use id field only, used for authorization
			requests like authorize or stopTransaction. If
			the authorization request is accepted, it is updated
			from trial to current. */
	} auth;

	struct {
		time_t started;
		time_t suspended;
		time_t expiry;
	} timestamp;

	bool remote_stop;
};

struct charger_ocpp_info {
	char msgid[OCPP_MESSAGE_ID_MAXLEN]; /* the latest message ID sent */

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

	struct fsm fsm;
	struct charging_session session;

	time_t now;
	bool *unavailable;

	struct charger_ocpp_info ocpp_info;
};

ocpp_connector_status_t get_status(const struct ocpp_connector *c);
const char *stringify_status(ocpp_connector_status_t status);

fsm_state_t s2cp(ocpp_connector_status_t status);

ocpp_status_t s2ocpp(ocpp_connector_status_t status);
ocpp_error_t e2ocpp(connector_error_t error);

bool is_occupied(const ocpp_connector_status_t status);
bool is_csms_up(const struct ocpp_connector *c);
bool is_session_established(const struct ocpp_connector *c);
bool is_session_active(const struct ocpp_connector *c);
bool is_session_trial_id_exist(const struct ocpp_connector *c);
bool is_transaction_started(const struct ocpp_connector *c);

void set_session_current_expiry(struct ocpp_connector *c, const time_t expiry);
void set_session_parent_id(struct ocpp_connector *c,
		const char id[OCPP_ID_TOKEN_MAXLEN]);
void set_session_current_id(struct ocpp_connector *c,
		const char id[OCPP_ID_TOKEN_MAXLEN]);
void set_session_trial_id(struct ocpp_connector *c,
		const char id[OCPP_ID_TOKEN_MAXLEN]);
void accept_session_trial_id(struct ocpp_connector *c);
void clear_session_id(struct auth *auth);
void clear_session(struct ocpp_connector *c);
void set_transaction_id(struct ocpp_connector *c,
		const ocpp_charger_transaction_id_t transaction_id);
void raise_event(struct ocpp_connector *c, const charger_event_t event);

ocpp_measurand_t update_metering(struct ocpp_connector *c);
ocpp_measurand_t update_metering_clock_aligned(struct ocpp_connector *c);

ocpp_stop_reason_t get_stop_reason(struct ocpp_connector *c,
		ocpp_connector_status_t status);

struct ocpp_connector *get_connector_by_message_id(struct ocpp_charger *self,
		const uint8_t *msgid, const size_t msgid_len);
struct ocpp_connector *get_connector_by_transaction_id(struct ocpp_charger *self,
		const ocpp_charger_transaction_id_t transaction_id);

const struct fsm_item *get_ocpp_fsm_table(size_t *transition_count);

#if defined(__cplusplus)
}
#endif

#endif /* OCPP_CONNECTOR_PRIVATE_H */
