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

#include "charger/free_connector.h"

#include <stdlib.h>
#include <errno.h>

#include "libmcu/fsm.h"
#include "libmcu/metrics.h"
#include "logger.h"

#if !defined(ARRAY_SIZE)
#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))
#endif

static bool is_initial(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	/* NOTE: Even if an E state occurs such as diode fault, since EVSE
	 * transitions to F state, the following condition is only satisfied
	 * during initial boot */
	return state == E &&
		connector_get_target_duty(c) == 0 &&
		connector_get_actual_duty(c) == 0;
}

static bool is_state_a(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	return connector_is_state_x(c, A);
}

static bool is_state_b(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	return connector_is_state_x(c, B);
}

static bool is_state_c(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	return connector_is_state_x(c, C);
}

static bool is_state_c2(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	return connector_is_state_x2(c, C);
}

static bool is_state_d2(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	return connector_is_state_x2(c, D);
}

static bool is_state_d(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	return connector_is_state_x(c, D);
}

static bool is_state_e(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	return connector_is_state_x(c, E);
}

static bool is_state_f(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	return connector_is_evse_error(c, (connector_state_t)state);
}

static bool is_recovered(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;

	if (!connector_is_input_power_ok(c) || connector_is_emergency_stop(c)) {
		return false;
	}

	const time_t now = time(NULL);
	const uint32_t elapsed = (uint32_t)(now - c->time_last_state_change);

	if (!connector_is_ev_response_timeout(c, elapsed)) {
		return false;
	}

	return true;
}

static void do_error(struct connector *c, connector_state_t state)
{
	if (connector_is_supplying_power(c)) {
		connector_disable_power_supply(c);
	}

	connector_go_fault(c);

	error("error state change: %s to %s", connector_stringify_state(state),
			connector_stringify_state(connector_state(c)));
}

static void do_stop_pwm(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	connector_stop_duty(c);
}

static void do_start_pwm(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	connector_start_duty(c);
}

static void do_supply_power(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	if (!connector_is_supplying_power(c)) {
		connector_enable_power_supply(c);
	}
}

static void do_stop_power(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	if (connector_is_supplying_power(c)) {
		connector_disable_power_supply(c);
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
	connector_go_fault(c);
	c->error = CONNECTOR_ERROR_EVSE_SIDE;

	metrics_increase(ChargerUnexpectedCount);
	error("unexpected state change: %s to %s",
			connector_stringify_state((connector_state_t)state),
			connector_stringify_state(connector_state(c)));
}

static void do_evse_error(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;

	do_error(c, (connector_state_t)state);
	c->error = connector_is_emergency_stop(c)?
		CONNECTOR_ERROR_EMERGENCY_STOP : CONNECTOR_ERROR_EVSE_SIDE;

	if (state == E || state == F) {
		error("unexpected evse error in %s state",
				connector_stringify_state((connector_state_t)
						state));
	}
}

static void do_ev_error(fsm_state_t state, fsm_state_t next_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;

	do_error(c, (connector_state_t)state);
	c->error = CONNECTOR_ERROR_EV_SIDE;

	if (state == A || state == E || state == F) {
		error("unexpected ev error in %s state",
				connector_stringify_state((connector_state_t)
						state));
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

static connector_event_t
get_event_from_state_change(const connector_state_t new_state,
		const connector_state_t old_state)
{
	const struct {
		connector_state_t from;
		connector_state_t to;
		uint32_t event;
	} tbl[] = {
		{ A, B, CONNECTOR_EVENT_PLUGGED },
		{ A, F, CONNECTOR_EVENT_ERROR },
		{ B, A, CONNECTOR_EVENT_UNPLUGGED },
		{ B, C, CONNECTOR_EVENT_CHARGING_STARTED },
		{ B, D, CONNECTOR_EVENT_CHARGING_STARTED },
		{ B, F, CONNECTOR_EVENT_ERROR },
		{ C, A, CONNECTOR_EVENT_CHARGING_ENDED | CONNECTOR_EVENT_UNPLUGGED },
		{ C, B, CONNECTOR_EVENT_CHARGING_ENDED },
		{ C, D, CONNECTOR_EVENT_NONE },
		{ C, E, CONNECTOR_EVENT_CHARGING_ENDED | CONNECTOR_EVENT_ERROR },
		{ C, F, CONNECTOR_EVENT_CHARGING_ENDED | CONNECTOR_EVENT_ERROR },
		{ D, A, CONNECTOR_EVENT_CHARGING_ENDED | CONNECTOR_EVENT_UNPLUGGED },
		{ D, B, CONNECTOR_EVENT_CHARGING_ENDED },
		{ D, C, CONNECTOR_EVENT_NONE },
		{ D, E, CONNECTOR_EVENT_CHARGING_ENDED | CONNECTOR_EVENT_ERROR },
		{ D, F, CONNECTOR_EVENT_CHARGING_ENDED | CONNECTOR_EVENT_ERROR },
		{ E, F, CONNECTOR_EVENT_ERROR },
		{ F, A, CONNECTOR_EVENT_ERROR_RECOVERY },
		{ F, B, CONNECTOR_EVENT_ERROR_RECOVERY | CONNECTOR_EVENT_PLUGGED },
	};

	for (size_t i = 0; i < ARRAY_SIZE(tbl); i++) {
		if (tbl[i].from == old_state && tbl[i].to == new_state) {
			return (connector_event_t)tbl[i].event;
		}
	}

	return CONNECTOR_EVENT_NONE;
}

static void update_metrics(const fsm_state_t state)
{
	const metric_key_t tbl[Sn] = {
		[A] = ChargerStateACount,
		[B] = ChargerStateBCount,
		[C] = ChargerStateCCount,
		[D] = ChargerStateDCount,
		[E] = ChargerStateECount,
		[F] = ChargerStateFCount,
	};
	metrics_increase(tbl[state]);
}

static void on_state_change(struct fsm *fsm,
		fsm_state_t new_state, fsm_state_t prev_state, void *ctx)
{
	struct connector *c = (struct connector *)ctx;
	c->time_last_state_change = time(NULL);

	info("connector \"%s\" state changed: %s to %s at %ld", c->param.name,
			connector_stringify_state((connector_state_t)prev_state),
			connector_stringify_state((connector_state_t)new_state),
			c->time_last_state_change);

	const connector_event_t events =
		get_event_from_state_change((connector_state_t)new_state,
				(connector_state_t)prev_state);

	if (events && c->event_cb) {
		(*c->event_cb)(c, events, c->event_cb_ctx);
	}

	update_metrics(new_state);
}

static int process(struct connector *self)
{
	fsm_step(&self->fsm);
	return 0;
}

static int enable(struct connector *self)
{
	(void)self;
	return 0;
}

static int disable(struct connector *self)
{
	(void)self;
	return 0;
}

static const struct connector_api *get_api(void)
{
	static struct connector_api api = {
		.enable = enable,
		.disable = disable,
		.process = process,
	};

	return &api;
}

struct connector *free_connector_create(const struct connector_param *param)
{
	struct connector *c;

	if (!param || !connector_validate_param(param) ||
			(c = malloc(sizeof(*c))) == NULL) {
		return NULL;
	}

	*c = (struct connector) {
		.api = get_api(),
		.param = *param,
	};

	fsm_init(&c->fsm, transitions, ARRAY_SIZE(transitions), c);
	fsm_set_state_change_cb(&c->fsm, on_state_change, c);

	ratelim_init(&c->log_ratelim, RATELIM_UNIT_SECOND,
			CONNECTOR_LOG_RATE_CAP, CONNECTOR_LOG_RATE_SEC);

	info("connector \"%s\" created", param->name);

	return c;
}

void free_connector_destroy(struct connector *c)
{
	free(c);
}
