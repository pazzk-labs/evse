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

#ifndef CONNECTOR_PRIVATE_H
#define CONNECTOR_PRIVATE_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "charger/connector.h"
#include "charger/event.h"

#include <time.h>

#include "libmcu/list.h"
#include "libmcu/fsm.h"

typedef enum {
	E, /* initial state */
	A,
	B,
	C,
	D,
	F,
	Sn, /* state count */
} connector_state_t;

struct connector {
	struct connector_param param;
	struct charger *charger;
	struct list link; /* to charger.connectors.list */
	int id;
	time_t time_last_state_change;

	connector_error_t error;

	struct fsm fsm;
};

connector_state_t get_state(const struct connector *c);
uint8_t get_pwm_duty_set(const struct connector *c);
uint8_t read_pwm_duty(const struct connector *c);

void start_pwm(struct connector *c);
void stop_pwm(struct connector *c);
void go_fault(struct connector *c);
void enable_power_supply(struct connector *c);
void disable_power_supply(struct connector *c);

bool is_state_x(const struct connector *c, connector_state_t state);
bool is_state_x2(const struct connector *c, connector_state_t state);
bool is_evse_error(const struct connector *c, connector_state_t state);

bool is_supplying_power(const struct connector *c);
bool is_emergency_stop(const struct connector *c);
bool is_input_power_ok(const struct connector *c);
bool is_output_power_ok(const struct connector *c);

bool is_early_recovery(uint32_t elapsed_sec);
charger_event_t get_event_from_state_change(const connector_state_t new_state,
		const connector_state_t old_state);
const char *stringify_state(const connector_state_t state);

void log_power_failure(const struct connector *c);

const struct fsm_item *get_fsm_table(size_t *transition_count);

#if defined(__cplusplus)
}
#endif

#endif /* CONNECTOR_PRIVATE_H */
