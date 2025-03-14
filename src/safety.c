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

#include "safety.h"

#include <string.h>
#include <errno.h>

#include "libmcu/metrics.h"
#include "libmcu/board.h"
#include "libmcu/ringbuf.h"
#include "libmcu/apptmr.h"
#include "usrinp.h"
#include "logger.h"

/* half of a cycle of 60Hz AC power. 8 milliseconds is conservative enough
 * and safe from overruns as we capture only falling edge. */
#define DEBOUNCE_DURATION_MS		8
#define FREQUENCY_TOLERANCE_HZ		3
#define MAX_SAMPLES			60
#define UPTODATE_DUE_MS			1000

struct waveform {
	uint32_t timestamp;
};

struct entry {
	struct gpio *gpio;
	struct ringbuf *ringbuf;
	uint32_t last_falling_time;
	uint32_t time_updated;

	uint8_t freq;
	int8_t missing_samples;

	safety_t type;
};

struct safety {
	struct entry output_power;
	struct entry input_power;
	struct apptmr *timer;

	bool emergency_stop_pressed;
};

static struct safety safety;

static size_t count_samples(struct entry *entry)
{
	return ringbuf_length(entry->ringbuf) / sizeof(struct waveform);
}

static bool is_pulse_active(struct entry *entry)
{
	return board_get_time_since_boot_ms() - entry->last_falling_time
			< UPTODATE_DUE_MS;
}

static bool is_uptodate(struct entry *entry)
{
	return board_get_time_since_boot_ms() - entry->time_updated
			< UPTODATE_DUE_MS * 2;
}

static void calc_freq(struct entry *entry,
		uint8_t *min, uint8_t *max, uint8_t *avg)
{
	size_t n = ringbuf_length(entry->ringbuf) / sizeof(struct waveform);
	uint32_t sum = 0;
	struct waveform older;

	for (size_t i = 0; i < n; i++) {
		struct waveform newer;
		if (ringbuf_read(entry->ringbuf, 0,
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

static void process_samples(struct entry *entry)
{
	uint8_t min = 0;
	uint8_t max = 0;

	if (!is_uptodate(entry)) {
		entry->freq = 0;
	}

	if (!is_pulse_active(entry)) {
		if (entry->type == SAFETY_TYPE_INPUT_POWER) {
			metrics_increase(SafetyNoPulseCount);
		}
		return;
	}
	if ((entry->missing_samples = (int8_t)
				(MAX_SAMPLES - count_samples(entry)))
			> FREQUENCY_TOLERANCE_HZ) {
		if (entry->type == SAFETY_TYPE_INPUT_POWER) {
			metrics_increase(SafetySamplingMissCount);
			error("missing samples: %d", entry->missing_samples);
		}
		return;
	}

	calc_freq(entry, &min, &max, &entry->freq);
	entry->time_updated = board_get_time_since_boot_ms();
	entry->missing_samples = 0;
}

static safety_status_t check_status(struct entry *entry,
		const uint8_t expected_freq)
{
	if (entry->type == SAFETY_TYPE_OUTPUT_POWER &&
			safety.emergency_stop_pressed) {
		return SAFETY_STATUS_EMERGENCY_STOP;
	}

	if (!is_uptodate(entry)) {
		return SAFETY_STATUS_STALE;
	}

	if (entry->missing_samples > FREQUENCY_TOLERANCE_HZ) {
		return SAFETY_STATUS_SAMPLING_ERROR;
	}

	if (entry->freq < expected_freq - FREQUENCY_TOLERANCE_HZ ||
			entry->freq > expected_freq + FREQUENCY_TOLERANCE_HZ) {
		return SAFETY_STATUS_ABNORMAL_FREQUENCY;
	}

	return SAFETY_STATUS_OK;
}

static void on_gpio_event(struct gpio *gpio, void *ctx)
{
	unused(gpio);
	struct entry *entry = (struct entry *)ctx;
	struct ringbuf *ringbuf = entry->ringbuf;
	const uint32_t t = board_get_time_since_boot_ms();

	if (t - entry->last_falling_time < DEBOUNCE_DURATION_MS) {
		metrics_increase(SafetyDebounceCount);
		return;
	}
	entry->last_falling_time = t;

	struct waveform sample = {
		.timestamp = t,
	};

	if (ringbuf_write(ringbuf, &sample, sizeof(sample)) == 0) { /* rotate */
		ringbuf_consume(ringbuf, sizeof(sample));
		if (ringbuf_write(ringbuf, &sample, sizeof(sample)) == 0) {
			metrics_increase(SafetyRingBufferOverflowCount);
		}
	}

	metrics_increase(SafetyInterruptCount);
}

static void on_emergency_stop(usrinp_event_t event, void *ctx)
{
	struct safety *self = (struct safety *)ctx;

	switch (event) {
	case USRINP_EVENT_CONNECTED:
		self->emergency_stop_pressed = true;
		metrics_increase(EmergencyStopCount);
		warn("Emergency stop button pressed");
		break;
	case USRINP_EVENT_DISCONNECTED:
		self->emergency_stop_pressed = false;
		metrics_increase(EmergencyReleaseCount);
		info("Emergency stop button released");
		break;
	default:
		break;
	}
}

static void on_periodic_timer(struct apptmr *timer, void *arg)
{
	unused(timer);
	struct safety *self = (struct safety *)arg;
	process_samples(&self->input_power);
	process_samples(&self->output_power);
}

safety_status_t safety_status(safety_t type, const uint8_t expected_freq)
{
	struct entry *entry = NULL;
	int n = 1;

	if (type == SAFETY_TYPE_INPUT_POWER) {
		entry = &safety.input_power;
	} else if (type == SAFETY_TYPE_OUTPUT_POWER) {
		entry = &safety.output_power;
	} else {
		entry = &safety.input_power;
		n = 2;
	}

	for (int i = 0; i < n; i++) {
		safety_status_t status = check_status(entry, expected_freq);
		if (status != SAFETY_STATUS_OK) {
			return status;
		}
		
		entry = &safety.output_power;
	}

	return SAFETY_STATUS_OK;
}

uint8_t safety_get_frequency(safety_t type)
{
	if (type == SAFETY_TYPE_INPUT_POWER) {
		return safety.input_power.freq;
	} else if (type == SAFETY_TYPE_OUTPUT_POWER) {
		return safety.output_power.freq;
	}
	return 0;
}

int safety_enable(void)
{
	gpio_register_callback(safety.output_power.gpio,
			on_gpio_event, &safety.output_power);
	gpio_register_callback(safety.input_power.gpio,
			on_gpio_event, &safety.input_power);
	gpio_enable(safety.output_power.gpio);
	gpio_enable(safety.input_power.gpio);

	apptmr_enable(safety.timer);
	apptmr_start(safety.timer, UPTODATE_DUE_MS);

	usrinp_register_event_cb(USRINP_EMERGENCY_STOP,
			on_emergency_stop, &safety);

	return 0;
}

int safety_disable(void)
{
	apptmr_stop(safety.timer);
	apptmr_disable(safety.timer);

	gpio_disable(safety.output_power.gpio);
	gpio_disable(safety.input_power.gpio);

	return 0;
}

int safety_init(struct gpio *input_power, struct gpio *output_power)
{
	safety.input_power.gpio = input_power;
	safety.output_power.gpio = output_power;

	safety.output_power.ringbuf =
		ringbuf_create(sizeof(struct waveform) * MAX_SAMPLES * 2);
	safety.input_power.ringbuf =
		ringbuf_create(sizeof(struct waveform) * MAX_SAMPLES * 2);
	safety.output_power.type = SAFETY_TYPE_OUTPUT_POWER;
	safety.input_power.type = SAFETY_TYPE_INPUT_POWER;

	if (safety.output_power.ringbuf == NULL ||
			safety.input_power.ringbuf == NULL) {
		return -ENOMEM;
	}

	if ((safety.timer = apptmr_create(true, on_periodic_timer, &safety))
			== NULL) {
		return -ENOMEM;
	}

	return 0;
}

void safety_deinit(void)
{
	if (safety.output_power.ringbuf) {
		ringbuf_destroy(safety.output_power.ringbuf);
	}

	if (safety.input_power.ringbuf) {
		ringbuf_destroy(safety.input_power.ringbuf);
	}

	if (safety.timer) {
		apptmr_delete(safety.timer);
	}

	memset(&safety, 0, sizeof(safety));
}
