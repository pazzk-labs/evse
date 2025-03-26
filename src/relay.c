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

#include "relay.h"

#include <string.h>

#include "libmcu/pwm.h"
#include "libmcu/apptmr.h"
#include "libmcu/metrics.h"

#if !defined(RELAY_MAX)
#define RELAY_MAX			1
#endif

/* NOTE: We are using AZSR250-2AE-12D.
 * pickup voltage: 9V which means 75% of 12V.
 * hold voltage: 5V which means 42% of 12V.
 */
#define PICKUP_MIN_PCT			75
#define PICKUP_DEFAULT_PCT		(PICKUP_MIN_PCT + 10)
#define PICKUP_MIN_DELAY_MS		20
#define PICKUP_DEFAULT_DELAY_MS		100
#define HOLD_MIN_PCT			42
#define HOLD_DEFAULT_PCT		(HOLD_MIN_PCT + 10)

#define PWM_FREQ			100000

#if !defined(MAX)
#define MAX(a, b)			((a) < (b) ? (b) : (a))
#endif

struct relay {
	struct lm_pwm_channel *pwm;
	struct apptmr *timer;
	uint8_t hold_duty_pct;
};

static struct relay *new_relay(struct relay *pool)
{
	struct relay empty = { 0, };

	for (size_t i = 0; i < RELAY_MAX; i++) {
		if (memcmp(&empty, &pool[i], sizeof(empty)) == 0) {
			return &pool[i];
		}
	}

	return NULL;
}

static void free_relay(struct relay *self)
{
	memset(self, 0, sizeof(*self));
}

static void on_timeout(struct apptmr *timer, void *arg)
{
	unused(timer);
	struct relay *p = (struct relay *)arg;
	const uint8_t pct = MAX(p->hold_duty_pct, HOLD_MIN_PCT);

	lm_pwm_update_duty(p->pwm, LM_PWM_PCT_TO_MILLI(pct));

	metrics_increase(RelayHoldCount);
	metrics_set(RelayHoldDuty, pct);
}

static void turn_on(struct relay *self, uint8_t duty_pct)
{
	lm_pwm_start(self->pwm, PWM_FREQ, LM_PWM_PCT_TO_MILLI(duty_pct));
	metrics_increase(RelayPickupCount);
	metrics_set(RelayPickupDuty, duty_pct);
}

static void turn_on_ext(struct relay *self,
		uint8_t pickup_pct, uint16_t pickup_delay_ms, uint8_t hold_pct)
{
	unused(hold_pct);
	turn_on(self, MAX(pickup_pct, PICKUP_MIN_PCT));

	apptmr_stop(self->timer);
	apptmr_start(self->timer, MAX(pickup_delay_ms, PICKUP_MIN_DELAY_MS));
}

/* TODO: Turn on relay when zero-crossing is detected to minimize arcing. */
void relay_turn_on_ext(struct relay *self,
		uint8_t pickup_pct, uint16_t pickup_delay_ms, uint8_t hold_pct)
{
	turn_on_ext(self, pickup_pct, pickup_delay_ms, hold_pct);
}

void relay_turn_on(struct relay *self)
{
#if 0
	turn_on(self, PICKUP_DEFAULT_PCT);
#else
	turn_on_ext(self, PICKUP_DEFAULT_PCT, PICKUP_DEFAULT_DELAY_MS,
			HOLD_DEFAULT_PCT);
#endif
}

void relay_turn_off(struct relay *self)
{
	apptmr_stop(self->timer);
	lm_pwm_stop(self->pwm);
	metrics_increase(RelayOffCount);
}

int relay_enable(struct relay *self)
{
	return lm_pwm_enable(self->pwm);
}

int relay_disable(struct relay *self)
{
	return lm_pwm_disable(self->pwm);
}

struct relay *relay_create(struct lm_pwm_channel *pwm_handle)
{
	static struct relay relays[RELAY_MAX];
	struct relay *p = new_relay(relays);

	if (p) {
		p->pwm = pwm_handle;
		p->timer = apptmr_create(false, on_timeout, p);

		if (p->timer == NULL) {
			free_relay(p);
			p = NULL;
		}
	}

	return p;
}

void relay_destroy(struct relay *self)
{
	free_relay(self);
}
