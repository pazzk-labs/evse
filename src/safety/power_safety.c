/*
 * This file is part of the Pazzk project <https://pazzk.net/>.
 * Copyright (c) 2025 Pazzk <team@pazzk.net>.
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

#include "power_safety.h"

#include <stdlib.h>
#include <string.h>

#include "libmcu/gpio.h"
#include "libmcu/ringbuf.h"
#include "libmcu/apptmr.h"
#include "libmcu/board.h"

#include "logger.h"

/* half of a cycle of 60Hz AC power. 8 milliseconds is conservative enough
 * and safe from overruns as we capture only falling edge. */
#define DEBOUNCE_DURATION_MS		8
#define FREQUENCY_TOLERANCE_HZ		3
#define MAX_SAMPLES			60
#define UPTODATE_DUE_MS			1000

struct safety_entry {
	struct safety_entry_api api;
	struct lm_gpio *gpio;
	int16_t expected_freq;

	struct ringbuf *ringbuf;
	uint32_t last_falling_time;
	uint32_t time_updated;

	uint8_t freq;
	int8_t missing_samples;

	struct apptmr *timer;
	const char *name;

	metric_key_t metrics[PowerMetricMax];
};

struct waveform {
	uint32_t timestamp;
};

static size_t count_samples(struct safety_entry *self)
{
	return ringbuf_length(self->ringbuf) / sizeof(struct waveform);
}

static bool is_pulse_active(struct safety_entry *self)
{
	return board_get_time_since_boot_ms() - self->last_falling_time
			< UPTODATE_DUE_MS;
}

static bool is_uptodate(struct safety_entry *self)
{
	return board_get_time_since_boot_ms() - self->time_updated
			< UPTODATE_DUE_MS * 2;
}

static void calc_freq(struct safety_entry *self,
		uint8_t *min, uint8_t *max, uint8_t *avg)
{
	size_t n = ringbuf_length(self->ringbuf) / sizeof(struct waveform);
	uint32_t sum = 0;
	struct waveform older;

	for (size_t i = 0; i < n; i++) {
		struct waveform newer;
		if (ringbuf_read(self->ringbuf, 0,
				&newer, sizeof(newer)) == 0) {
			n = i;
			break;
		} else if (i == 0) {
			older = newer;
			continue;
		}

		const uint32_t diff = newer.timestamp - older.timestamp;
		const uint8_t freq = (uint8_t)(diff? 1000 / diff : 0);

		if (sum == 0) {
			*min = freq;
			*max = freq;
		} else {
			if (freq < *min) {
				*min = freq;
			}
			if (freq > *max) {
				*max = freq;
			}
		}

		sum += (uint32_t)freq;
		older = newer;
	}

	*avg = (uint8_t)(n? (sum*10 / n + 5) / 10 : 0);
}

static void process_samples(struct safety_entry *self)
{
	uint8_t min = 0;
	uint8_t max = 0;

	if (!is_uptodate(self)) {
		self->freq = 0;
	}

	if (!is_pulse_active(self)) {
		metrics_increase(self->metrics[PowerMetricNoPulseCount]);
		return;
	}
	if ((self->missing_samples = (int8_t)(MAX_SAMPLES - count_samples(self)))
			> FREQUENCY_TOLERANCE_HZ) {
		metrics_increase(self->metrics[PowerMetricSamplingMissCount]);
		error("missing samples: %d", self->missing_samples);
		return;
	}

	calc_freq(self, &min, &max, &self->freq);
	self->time_updated = board_get_time_since_boot_ms();
	self->missing_samples = 0;
}

static void on_periodic_timer(struct apptmr *timer, void *arg)
{
	unused(timer);
	struct safety_entry *self = (struct safety_entry *)arg;
	process_samples(self);
}

static void on_gpio_event(struct lm_gpio *gpio, void *ctx)
{
	unused(gpio);
	struct safety_entry *self = (struct safety_entry *)ctx;
	struct ringbuf *ringbuf = self->ringbuf;
	const uint32_t t = board_get_time_since_boot_ms();

	if (t - self->last_falling_time < DEBOUNCE_DURATION_MS) {
		metrics_increase(self->metrics[PowerMetricDebounceCount]);
		return;
	}
	self->last_falling_time = t;

	struct waveform sample = {
		.timestamp = t,
	};

	if (ringbuf_write(ringbuf, &sample, sizeof(sample)) == 0) { /* rotate */
		ringbuf_consume(ringbuf, sizeof(sample));
		if (ringbuf_write(ringbuf, &sample, sizeof(sample)) == 0) {
			metrics_increase(self->metrics[PowerMetricRingBufferOverflowCount]);
		}
	}

	metrics_increase(self->metrics[PowerMetricInterruptCount]);
}

static safety_entry_status_t check(struct safety_entry *self)
{
	if (!is_uptodate(self)) {
		return SAFETY_STATUS_STALE;
	}

	if (self->missing_samples > FREQUENCY_TOLERANCE_HZ) {
		return SAFETY_STATUS_SAMPLING_ERROR;
	}

	if (self->freq < self->expected_freq - FREQUENCY_TOLERANCE_HZ ||
			self->freq > self->expected_freq + FREQUENCY_TOLERANCE_HZ) {
		return SAFETY_STATUS_ABNORMAL_FREQUENCY;
	}

	return SAFETY_STATUS_OK;
}

static int get_frequency(const struct safety_entry *self)
{
	return (int)self->freq;
}

static const char *get_name(const struct safety_entry *self)
{
	return self->name;
}

static int enable(struct safety_entry *self)
{
	lm_gpio_register_callback(self->gpio, on_gpio_event, self);
	lm_gpio_enable(self->gpio);

	apptmr_enable(self->timer);
	apptmr_start(self->timer, UPTODATE_DUE_MS);

	return 0;
}

static int disable(struct safety_entry *self)
{
	apptmr_stop(self->timer);
	apptmr_disable(self->timer);

	lm_gpio_disable(self->gpio);

	return 0;
}

static void destroy(struct safety_entry *self)
{
	apptmr_delete(self->timer);
	ringbuf_destroy(self->ringbuf);
}

struct safety_entry *power_safety_create(const char *name, struct lm_gpio *gpio,
		int16_t freq, const metric_key_t *metrics)
{
	struct safety_entry *power =
		(struct safety_entry *)malloc(sizeof(struct safety_entry));

	if (!power) {
		return NULL;
	}

	*power = (struct safety_entry) {
		.gpio = gpio,
		.expected_freq = freq,
		.name = name,
		.api = {
			.enable = enable,
			.disable = disable,
			.check = check,
			.get_frequency = get_frequency,
			.name = get_name,
			.destroy = destroy,
		},
	};

	memcpy(power->metrics, metrics, sizeof(power->metrics));

	if ((power->ringbuf = ringbuf_create(sizeof(struct waveform)
			* MAX_SAMPLES * 2)) == NULL) {
		free(power);
		error("Failed to create ring buffer");
		return NULL;
	}
	if ((power->timer = apptmr_create(true, on_periodic_timer, power))
			== NULL) {
		ringbuf_destroy(power->ringbuf);
		free(power);
		error("Failed to create timer");
		return NULL;
	}

	return power;
}
