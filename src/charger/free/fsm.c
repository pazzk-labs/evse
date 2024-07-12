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

#include "../connector_private.h"

#include <string.h>
#include <stdlib.h>

#include "libmcu/fsm.h"
#include "libmcu/metrics.h"
#include "logger.h"

#if !defined(ARRAY_SIZE)
#define ARRAY_SIZE(x)			(sizeof(x) / sizeof((x)[0]))
#endif

static bool is_initial(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	return state == E && get_pwm_duty_set(c) == 0 && read_pwm_duty(c) == 0;
}

static bool is_state_a(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	return is_state_x(c, A);
}

static bool is_state_b(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	return is_state_x(c, B);
}

static bool is_state_c(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	return is_state_x(c, C);
}

static bool is_state_c2(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	return is_state_x2(c, C);
}

static bool is_state_d2(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	return is_state_x2(c, D);
}

static bool is_state_d(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	return is_state_x(c, D);
}

static bool is_state_e(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	return is_state_x(c, E);
}

static bool is_state_f(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	return is_evse_error(c, (connector_state_t)state);
}

static bool is_recovered(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;

	if (!is_input_power_ok(c) || is_emergency_stop(c)) {
		return false;
	}

	const time_t now = time(NULL);
	const uint32_t elapsed = (uint32_t)(now - c->time_last_state_change);

	if (is_early_recovery(elapsed)) {
		return false;
	}

	return true;
}

static void do_error(struct connector *c, connector_state_t state)
{
	if (is_supplying_power(c)) {
		disable_power_supply(c);
	}

	go_fault(c);

	error("error state change: %s to %s",
			stringify_state(state), stringify_state(get_state(c)));
}

static void do_stop_pwm(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	stop_pwm(c);
}

static void do_start_pwm(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	start_pwm(c);
}

static void do_supply_power(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	if (!is_supplying_power(c)) {
		enable_power_supply(c);
	}
}

static void do_stop_power(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	if (is_supplying_power(c)) {
		disable_power_supply(c);
	}
}

static void do_stop_all(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	do_stop_power(state, next_state, ctx);
	do_stop_pwm(state, next_state, ctx);
}

static void do_unexpected(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	go_fault(c);
	c->error = CONNECTOR_ERROR_EVSE_SIDE;

	metrics_increase(ChargerUnexpectedCount);
	error("unexpected state change: %s to %s",
			stringify_state((connector_state_t)state),
			stringify_state(get_state(c)));
}

static void do_evse_error(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;

	do_error(c, (connector_state_t)state);
	c->error = is_emergency_stop(c)?
		CONNECTOR_ERROR_EMERGENCY_STOP : CONNECTOR_ERROR_EVSE_SIDE;

	if (!is_input_power_ok(c) ||
			(c->error != CONNECTOR_ERROR_EMERGENCY_STOP &&
					!is_output_power_ok(c))) {
		log_power_failure(c);
	}

	if (state == E || state == F) {
		error("unexpected evse error in %s state",
				stringify_state((connector_state_t)state));
	}
}

static void do_ev_error(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;

	do_error(c, (connector_state_t)state);
	c->error = CONNECTOR_ERROR_EV_SIDE;

	if (state == A || state == E || state == F) {
		error("unexpected ev error in %s state",
				stringify_state((connector_state_t)state));
	}
}

static const struct fsm_item transitions[] = {
	FSM_ITEM(E, is_initial,   do_stop_pwm,     A),

	FSM_ITEM(A, is_state_b,   do_start_pwm,    B), /* plugged */
	FSM_ITEM(A, is_state_c,   do_unexpected,   F),
	FSM_ITEM(A, is_state_d,   do_unexpected,   F),
	FSM_ITEM(A, is_state_e,   do_unexpected,   F), /* power off */
	FSM_ITEM(A, is_state_f,   do_evse_error,   F),

	FSM_ITEM(B, is_state_a,   do_stop_pwm,     A), /* unplugged */
	FSM_ITEM(B, is_state_c2,  do_supply_power, C), /* EV ready to charge */
	FSM_ITEM(B, is_state_d2,  do_supply_power, D), /* EV ready to charge */
	FSM_ITEM(B, is_state_e,   do_ev_error,     F),
	FSM_ITEM(B, is_state_f,   do_evse_error,   F),

	FSM_ITEM(C, is_state_a,   do_stop_all,     A), /* unplugged */
	FSM_ITEM(C, is_state_b,   do_stop_power,   B), /* EV suspended */
	FSM_ITEM(C, is_state_d,   NULL,            D), /* nothing to be done */
	FSM_ITEM(C, is_state_e,   do_ev_error,     F),
	FSM_ITEM(C, is_state_f,   do_evse_error,   F),

	FSM_ITEM(D, is_state_a,   do_stop_all,     A), /* unplugged */
	FSM_ITEM(D, is_state_b,   do_stop_power,   B), /* EV suspended */
	FSM_ITEM(D, is_state_c,   NULL,            C), /* nothing to be done */
	FSM_ITEM(D, is_state_e,   do_ev_error,     F),
	FSM_ITEM(D, is_state_f,   do_evse_error,   F),

	FSM_ITEM(F, is_state_a,   do_unexpected,   F),
	FSM_ITEM(F, is_state_b,   do_unexpected,   F),
	FSM_ITEM(F, is_state_c,   do_unexpected,   F),
	FSM_ITEM(F, is_state_d,   do_unexpected,   F),
	FSM_ITEM(F, is_state_e,   do_unexpected,   F),
	FSM_ITEM(F, is_recovered, do_stop_pwm,     A), /* EVSE recovery */
};

const struct fsm_item *get_fsm_table(size_t *transition_count)
{
	if (transition_count) {
		*transition_count = ARRAY_SIZE(transitions);
	}

	return transitions;
}
