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

#include "pilot.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libmcu/apptmr.h"
#include "libmcu/metrics.h"
#include "libmcu/board.h"
#include "libmcu/wdt.h"
#include "libmcu/ratelim.h"
#include "libmcu/pwm.h"

#include "adc122s051.h"
#include "logger.h"

#define CP_FREQ				1000

/* NOTE: keep the number of waveform buffers at least more or equal to 3 to not
 * overwrite the one that is being used. The one will not be overwritten for 20
 * milliseconds with 3 buffers as the interval of sampling is 10 milliseconds.
 * Increase the number of buffers if shorter interval is required or API calls
 * can not be finished in time due to any kind of latency. This is for
 * lock-free programming. */
#define MAX_WAVEFORMS			3

#define PILOT_WDT_TIMEOUT_MS		500

#define LOG_RATE_CAP			5
#define LOG_RATE_MIN			2

#if !defined(MAX)
#define MAX(a, b)			(((a) < (b))? (b) : (a))
#endif

struct buffer {
	uint16_t *data;
	uint16_t count;
};

struct waveform {
	uint16_t highs;
	uint16_t lows;
	uint16_t highs_outliers;
	uint16_t lows_outliers;
	uint16_t highs_max;
	uint16_t lows_min;
	uint32_t highs_sum;
	uint32_t lows_sum;

	struct buffer *high_samples;
	struct buffer *low_samples;
};

struct pilot {
	struct adc122s051 *adc;
	struct pwm_channel *pwm;
	struct apptmr *timer;
	struct wdt *wdt;
	struct ratelim log_ratelim;

	struct pilot_params params;

	pilot_status_cb_t cb;
	void *cb_ctx;

	struct {
		struct buffer samples; /* all samples */
		struct buffer highs; /* high samples */
		struct buffer lows; /* low samples */
	} buffer;

	struct {
		struct waveform measured; /* for internal use only */
		struct waveform saved[MAX_WAVEFORMS]; /* buffers */
		struct waveform *cached; /* for api calls.
					    will not be touched by any other */
	} waveform;

	uint32_t timestamp;
	uint8_t duty_pct;

	pilot_status_t status;

	bool running;
};

static uint16_t get_average(const uint32_t sum, const uint16_t count)
{
	if (count) {
		return (uint16_t)(sum / count);
	}
	return 0;
}

static uint32_t sqrt_u32(const uint32_t val)
{
	if (val == 0) {
		return 0;
	}

	uint32_t x = val;
	uint32_t y = 1;

	while (x > y) {
		x = (x + y) / 2;
		y = val / x;
	}

	return x;
}

static uint16_t calculate_standard_deviation(const uint16_t *data,
		const size_t data_count, const uint16_t avg)
{
	if (data_count == 0 || avg == 0) {
		return 0;
	}

	uint32_t variance = 0;

	for (size_t i = 0; i < data_count; i++) {
		int32_t diff = (int32_t)(data[i] - avg);
		variance += (uint32_t)(diff * diff);
	}

	return (uint16_t)sqrt_u32(variance / (uint32_t)data_count);
}

static void clear_waveform(struct waveform *waveform)
{
	struct buffer *highs = waveform->high_samples;
	struct buffer *lows = waveform->low_samples;

	if (highs) {
		memset(highs->data, 0, sizeof(*highs->data) * highs->count);
		highs->count = 0;
	}
	if (lows) {
		memset(lows->data, 0, sizeof(*lows->data) * lows->count);
		lows->count = 0;
	}

	waveform->highs = 0;
	waveform->lows = 0;
	waveform->highs_sum = 0;
	waveform->lows_sum = 0;
	waveform->highs_outliers = 0;
	waveform->lows_outliers = 0;
	waveform->highs_max = 0;
	waveform->lows_min = 0;
}

static struct waveform *get_next_cache(struct pilot *pilot)
{
	for (size_t i = 0; i < MAX_WAVEFORMS; i++) {
		if (pilot->waveform.cached == &pilot->waveform.saved[i]) {
			return &pilot->waveform.saved[(i + 1) % MAX_WAVEFORMS];
		}
	}

	return &pilot->waveform.saved[0];
}

static void update_status(struct pilot *pilot, const pilot_status_t status)
{
	if (pilot->status != status) {
		pilot->status = status;
		if (pilot->cb) {
			(*pilot->cb)(pilot->cb_ctx, status);
		}
	}
}

static void update_cache(struct pilot *pilot)
{
	struct waveform *next = get_next_cache(pilot);
	memcpy(next, &pilot->waveform.measured,
			sizeof(pilot->waveform.measured));
	pilot->waveform.cached = next;
}

static uint8_t get_duty(const struct waveform *waveform)
{
	if (!waveform) {
		return 0;
	}

        /* Split in half because both of rising and falling transitions are
         * counted.
	 *
	 * Note that all outliers are considered as the result of transitions.
	 * Outliers from noise are just ignored. */
	const uint16_t transitions = waveform->lows_outliers / 2 +
		waveform->highs_outliers / 2;
	const uint16_t total = (uint16_t)(waveform->highs + waveform->lows +
		waveform->highs_outliers + waveform->lows_outliers);
	const uint16_t highs = waveform->highs + transitions;

	if (total == 0) {
		return 0;
	}

	/* round to the nearest integer. */
	return (uint8_t)(((highs * 1000 / total) + 5) / 10);
}

static bool is_duty_as_expected(const uint8_t expected,
		const struct waveform *waveform)
{
	const uint8_t measured = get_duty(waveform);
	const uint8_t allowed_error = 1; /* 1% error is allowed */

	if (measured > expected + allowed_error ||
			measured < expected - allowed_error) {
		return false;
	}

	return true;
}

static bool is_within_boundary(const struct waveform *waveform,
		const struct pilot_boundaries *lim)
{
	const uint32_t high = waveform->highs_max;
	const uint32_t low = waveform->lows_min;

	if ((high <= lim->upward.a && high >= lim->downward.a) ||
			(low <= lim->upward.a && low >= lim->downward.a)) {
		return false;
	} else if ((high <= lim->upward.b && high >= lim->downward.b) ||
			(low <= lim->upward.b && low >= lim->downward.b)) {
		return false;
	} else if ((high <= lim->upward.c && high >= lim->downward.c) ||
			(low <= lim->upward.c && low >= lim->downward.c)){
		return false;
	} else if ((high <= lim->upward.d && high >= lim->downward.d) ||
			(low <= lim->upward.d && low >= lim->downward.d)) {
		return false;
	}

	return true;
}

static bool is_anomaly(const struct waveform *measured,
		const struct waveform *cached,
		const uint16_t tolerance_mv,
		const uint16_t max_transition_clocks)
{
	const uint32_t new = measured->highs_max;
	const uint32_t old = cached->highs_max;

	int32_t diff = (int32_t)(new - old);

	if (diff < 0) {
		diff = -diff;
	}

	if (diff > tolerance_mv) {
		return true;
	}

	if (measured->highs_outliers >= max_transition_clocks ||
			measured->lows_outliers >= max_transition_clocks) {
		return true;
	}

	return false;
}

static pilot_error_t check_error(const struct pilot *pilot)
{
	const uint32_t t = (uint32_t)board_get_time_since_boot_ms();
	const struct waveform *waveform = pilot->waveform.cached;

        /* if not measured more than 2*scan_interval_ms, something is wrong:
         * either the timer is not running or the adc error. */
	if (t - pilot->timestamp > 2 * pilot->params.scan_interval_ms) {
		return PILOT_ERROR_TOO_LONG_INTERVAL;
	}

	if (!is_duty_as_expected(pilot->duty_pct, waveform)) {
		return PILOT_ERROR_DUTY_MISMATCH;
	}

	if (waveform && !is_within_boundary(waveform,
			&pilot->params.boundary)) {
		return PILOT_ERROR_FLUCTUATING;
	}

	return PILOT_ERROR_NONE;
}

static void update_metric(const uint16_t min, const uint16_t max,
		const uint32_t t0, const uint32_t t1)
{
	const uint32_t diff = t1 - t0;

	metrics_set_if_max(max, METRICS_VALUE(diff));
	metrics_set_if_min(min, METRICS_VALUE(diff));
}

static void remove_outliers(uint16_t *samples, uint32_t *samples_sum,
		uint16_t *samples_count, uint16_t *outliers,
		uint16_t *max, uint16_t *min, const uint16_t tolerance_mv)
{
	if (*samples_count == 0) {
		return;
	}

	const uint16_t avg = get_average(*samples_sum, *samples_count);
	const uint16_t stdev = MAX(calculate_standard_deviation(samples,
			*samples_count, avg), tolerance_mv);

	if (max) {
		*max = samples[0];
	}
	if (min) {
		*min = samples[0];
	}

	for (size_t i = 0; i < *samples_count; i++) {
		const uint16_t val = samples[i];
		int16_t diff = (int16_t)(val - avg);
		diff = diff < 0 ? (int16_t)-diff : diff;
		if ((uint16_t)diff > stdev) {
			*samples_sum -= val;
			*outliers += 1;
			metrics_increase(PilotOutlierCount);
		} else {
			if (max && val > *max) {
				*max = val;
			}
			if (min && val < *min) {
				*min = val;
			}
		}
	}

	*samples_count -= *outliers;
}

static void do_post_process(struct waveform *waveform,
		const uint16_t tolerance_mv)
{
	remove_outliers(waveform->high_samples->data, &waveform->highs_sum,
			&waveform->highs, &waveform->highs_outliers,
			&waveform->highs_max, NULL, tolerance_mv);

	remove_outliers(waveform->low_samples->data, &waveform->lows_sum,
			&waveform->lows, &waveform->lows_outliers,
			NULL, &waveform->lows_min, tolerance_mv);
}

static pilot_status_t evaluate_status(const struct waveform *measured,
		const struct pilot_boundary *lim)
{
	const uint32_t high = measured->highs_max;
	const uint32_t low = measured->lows_min;
	pilot_status_t status;

	if (high > lim->a) {
		status = PILOT_STATUS_A;
	} else if (high > lim->b) {
		status = PILOT_STATUS_B;
	} else if (high > lim->c) {
		status = PILOT_STATUS_C;
	} else if (high > lim->d) {
		status = PILOT_STATUS_D;
	} else {
		status = PILOT_STATUS_F;
	}

	if (low > lim->e) { /* diode fault */
		status = PILOT_STATUS_E;
	}

	return status;
}

static uint16_t on_adc_sample(void *ctx, const uint16_t adc)
{
	struct pilot *pilot = (struct pilot *)ctx;
	struct waveform *waveform = &pilot->waveform.measured;
	struct buffer *highs = waveform->high_samples;
	struct buffer *lows = waveform->low_samples;

	const uint16_t millivolt = adc122s051_convert_adc_to_millivolt(adc);

	if (millivolt > pilot->params.cutoff_voltage_mv) {
		waveform->highs++;
		waveform->highs_sum += millivolt;

		if (highs) {
			highs->data[highs->count++] = millivolt;
		}
	} else {
		waveform->lows++;
		waveform->lows_sum += millivolt;

		if (lows) {
			lows->data[lows->count++] = millivolt;
		}
	}

	return millivolt;
}

static bool measure(struct pilot *pilot)
{
	struct buffer *buffer = &pilot->buffer.samples;
	struct waveform *waveform = &pilot->waveform.measured;

	clear_waveform(waveform);
	buffer->count = pilot->params.sample_count;

	const uint64_t t0 = board_get_time_since_boot_us();
	if (adc122s051_measure(pilot->adc, buffer->data, buffer->count,
			&(struct adc122s051_callback) {
				.on_sample = on_adc_sample,
				.on_sample_ctx = pilot}) != 0) {
		metrics_increase(PilotReadErrorCount);
		return false;
	}
	const uint64_t t1 = board_get_time_since_boot_us();

	do_post_process(waveform, pilot->params.noise_tolerance_mv);

	if (!is_duty_as_expected(pilot->duty_pct, waveform)) {
		ratelim_request_format(&pilot->log_ratelim, logger_warn,
				"duty cycle is not as expected: %u%% != %u%%",
				get_duty(waveform), pilot->duty_pct);
		metrics_increase(PilotDutyErrorCount);
	}
	if (!is_within_boundary(waveform, &pilot->params.boundary)) {
		ratelim_request_format(&pilot->log_ratelim, logger_warn,
				"CP is not within the boundary: %umV, %umV",
				waveform->highs_max, waveform->lows_min);
		metrics_increase(PilotBoundaryValueCount);
	}

	metrics_increase(PilotMeasureCount);
	update_metric(PilotMeasureTimeMin, PilotMeasureTimeMax,
			(uint32_t)t0, (uint32_t)t1);

	return true;
}

/* FIXME: it takes 2ms to 3ms to finish the job, which is too long in the
 * context of the 10ms interval of the timer. */
static void on_timeout(struct apptmr *timer, void *arg)
{
	struct pilot *p = (struct pilot *)arg;
	const pilot_status_t prev_status = p->status;
	const uint32_t t = (uint32_t)board_get_time_since_boot_ms();

	wdt_feed(p->wdt);

	update_metric(PilotIntervalMin, PilotIntervalMax, p->timestamp, t);
	p->timestamp = t;

	if (p->running) { /* the previous job is not done yet */
		metrics_increase(PilotOverrunCount);
		return;
	}

	/* FIXME: memory barrier for running flag to be visible. */
	p->running = true;
	{
		if (measure(p)) {
			const struct waveform *w = &p->waveform.measured;
			const struct pilot_boundaries *b = &p->params.boundary;
			pilot_status_t new_status =
				evaluate_status(w, &b->downward);

			if (new_status > prev_status) {
				new_status = evaluate_status(w, &b->upward);
			}

			const bool changed = new_status != prev_status;
			if (!changed && is_anomaly(&p->waveform.measured,
					p->waveform.cached,
					p->params.noise_tolerance_mv,
					p->params.max_transition_clocks)) {
				metrics_increase(PilotAnomalyCount);
			}

			update_cache(p);
			update_status(p, new_status);
		}
	}
	p->running = false;
}

static int init_pwm(struct pwm_channel *pwm)
{
	int err = pwm_enable(pwm);
	err |= pwm_start(pwm, CP_FREQ, PWM_PCT_TO_MILLI(0));
	return err;
}

static void set_params(struct pilot_params *dst, const struct pilot_params *src)
{
	memcpy(dst, src, sizeof(*dst));
}

int pilot_set_duty(struct pilot *pilot, const uint8_t pct)
{
	pilot->duty_pct = pct;
	return pwm_update_duty(pilot->pwm, PWM_PCT_TO_MILLI(pct));
}

uint8_t pilot_get_duty_set(const struct pilot *pilot)
{
	return pilot->duty_pct;
}

uint8_t pilot_duty(const struct pilot *pilot)
{
	return get_duty(pilot->waveform.cached);
}

pilot_status_t pilot_status(const struct pilot *pilot)
{
	return pilot->status;
}

uint16_t pilot_millivolt(const struct pilot *pilot, const bool low_voltage)
{
	const struct waveform *waveform = pilot->waveform.cached;

	if (low_voltage) {
		if (waveform && waveform->lows) {
			return waveform->lows_min;
		}
		return 0;
	}

	if (waveform && waveform->highs) {
		return waveform->highs_max;
	}

	return 0;
}

bool pilot_ok(const struct pilot *pilot)
{
	return check_error(pilot) == PILOT_ERROR_NONE;
}

pilot_error_t pilot_error(const struct pilot *pilot)
{
	return check_error(pilot);
}

const char *pilot_stringify_status(const pilot_status_t status)
{
	switch (status) {
	case PILOT_STATUS_A:
		return "A(12V)";
	case PILOT_STATUS_B:
		return "B(9V)";
	case PILOT_STATUS_C:
		return "C(6V)";
	case PILOT_STATUS_D:
		return "D(3V)";
	case PILOT_STATUS_E:
		return "E(0V)";
	case PILOT_STATUS_F:
		return "F(-12V)";
	default:
		return "Unknown";
	}
}

void pilot_default_params(struct pilot_params *params)
{
	if (params == NULL) {
		return;
	}

	/* refer to docs/control-pilot.md for the details of the parameters. */
	*params = (struct pilot_params) {
		.scan_interval_ms = 10,
		.cutoff_voltage_mv = 1996, /* CP_1V_mV */
		.noise_tolerance_mv = 50,
		.max_transition_clocks = 15, /* 30us as 1 sample time is 2us */
		.boundary = {
			.upward = {
				.a = 3038, /* 10.75V */
				.b = 2718, /* 7.75V */
				.c = 2397, /* 4.75V */
				.d = 2076, /* 1.75V */
				.e = 767, /* -10.5V */
			},
			.downward = {
				.a = 2985, /* 10.25V */
				.b = 2644, /* 7.25V */
				.c = 2344, /* 4.25V */
				.d = 2022, /* 1.25V */
				.e = 767, /* -10.5V */
			},
		},
		.sample_count = 500, /* 500 samples per 1ms, which is a cycle
					of 1kHz, at 8MHz 500kSPS ADC. */
	};
}

void pilot_set_status_cb(struct pilot *pilot,
		pilot_status_cb_t cb, void *cb_ctx)
{
	pilot->cb = cb;
	pilot->cb_ctx = cb_ctx;
}

int pilot_enable(struct pilot *pilot)
{
	int err = init_pwm(pilot->pwm);

	if (!err) {
		if ((err = apptmr_enable(pilot->timer))) {
			return err;
		}

		pilot->timestamp = (uint32_t)board_get_time_since_boot_ms();
		apptmr_start(pilot->timer, pilot->params.scan_interval_ms);

		wdt_enable(pilot->wdt);
	}

	return err;
}

int pilot_disable(struct pilot *pilot)
{
	wdt_disable(pilot->wdt);

	int err = apptmr_stop(pilot->timer);
	err |= apptmr_disable(pilot->timer);
	err |= pwm_stop(pilot->pwm);
	err |= pwm_disable(pilot->pwm);
	return err;
}

struct pilot *pilot_create(const struct pilot_params *params,
        struct adc122s051 *adc, struct pwm_channel *pwm, uint16_t *buf)
{
	static struct pilot pilot;
	struct pilot *p = &pilot;

	memset(p, 0, sizeof(*p));
	set_params(&p->params, params);

	if ((p->wdt = wdt_new("pilot", PILOT_WDT_TIMEOUT_MS, 0, 0)) == NULL) {
		return NULL;
	}
	if ((p->timer = apptmr_create(true, on_timeout, p)) == NULL) {
		wdt_delete(p->wdt);
		return NULL;
	}

	const size_t buffer_size = sizeof(*buf) * p->params.sample_count;
	if ((p->buffer.highs.data = malloc(buffer_size))) {
		p->waveform.measured.high_samples = &p->buffer.highs;
	}
	if ((p->buffer.lows.data = malloc(buffer_size))) {
		p->waveform.measured.low_samples = &p->buffer.lows;
	}

	p->pwm = pwm;
	p->adc = adc;
	p->buffer.samples.data = buf;
	p->status = PILOT_STATUS_UNKNOWN;

	ratelim_init(&p->log_ratelim,
			RATELIM_UNIT_MINUTE, LOG_RATE_CAP, LOG_RATE_MIN);

	info("pilot created: %u samples every %u ms",
			p->params.sample_count, p->params.scan_interval_ms);

	debug("\tcutoff voltage: %u mV", p->params.cutoff_voltage_mv);
	debug("\tnoise tolerance: %u mV", p->params.noise_tolerance_mv);
	debug("\tmax transition clocks: %u", p->params.max_transition_clocks);
	debug("\tupward boundary: %u, %u, %u, %u, %u",
			p->params.boundary.upward.a,
			p->params.boundary.upward.b,
			p->params.boundary.upward.c,
			p->params.boundary.upward.d,
			p->params.boundary.upward.e);
	debug("\tdownward boundary: %u, %u, %u, %u, %u",
			p->params.boundary.downward.a,
			p->params.boundary.downward.b,
			p->params.boundary.downward.c,
			p->params.boundary.downward.d,
			p->params.boundary.downward.e);

	return p;
}

void pilot_delete(struct pilot *pilot)
{
	if (pilot->buffer.lows.data) {
		free(pilot->buffer.lows.data);
	}
	if (pilot->buffer.highs.data) {
		free(pilot->buffer.highs.data);
	}

	apptmr_delete(pilot->timer);
	wdt_delete(pilot->wdt);
}
