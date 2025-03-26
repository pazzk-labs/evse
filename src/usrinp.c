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

#include "usrinp.h"

#include <pthread.h>
#include <semaphore.h>

#include "libmcu/gpio.h"
#include "libmcu/button.h"
#include "libmcu/board.h"
#include "libmcu/timext.h"
#include "libmcu/metrics.h"
#include "logger.h"

#define STACK_SIZE_BYTES	4096U

#if !defined(MIN)
#define MIN(a, b)		(((a) > (b))? (b) : (a))
#endif

struct debug_button {
	struct lm_gpio *gpio;
	struct button *button;
	struct button_param param;

	usrinp_event_cb_t cb;
	void *cb_ctx;

	bool raised;
};

struct emergency_stop {
	struct button *button;
	struct button_param param;

	usrinp_event_cb_t cb;
	void *cb_ctx;

	bool raised;
};

struct usb_vbus {
	struct button *button;
	struct button_param param;

	usrinp_event_cb_t cb;
	void *cb_ctx;

	bool raised;
};

struct usrinp {
	struct debug_button dbgbtn;
	struct emergency_stop estop;
	struct usb_vbus usb;

	usrinp_get_state_t get_state;

	pthread_t thread;
	sem_t poll;
};

static struct usrinp usrinp;

static void on_gpio_intr(struct lm_gpio *gpio, void *ctx)
{
	struct usrinp *self = (struct usrinp *)ctx;

	sem_post(&self->poll);

	lm_gpio_disable_interrupt(gpio);
	self->dbgbtn.raised = true;
}

static void process_event(const button_state_t event,
		usrinp_event_cb_t cb, void *cb_ctx)
{
	usrinp_event_t usrevt = USRINP_EVENT_UNKNOWN;

	switch (event) {
	case BUTTON_STATE_PRESSED:
		usrevt = USRINP_EVENT_CONNECTED;
		break;
	case BUTTON_STATE_RELEASED:
		usrevt = USRINP_EVENT_DISCONNECTED;
		break;
	default:
		break;
	}

	if (cb) {
		(*cb)(usrevt, cb_ctx);
	}
}

static void on_emergency_stop(struct button *btn,
		const button_state_t event, const uint16_t clicks,
		const uint16_t repeats, void *ctx)
{
	unused(btn);
	unused(clicks);
	unused(repeats);
	struct emergency_stop *self = (struct emergency_stop *)ctx;
	process_event(event, self->cb, self->cb_ctx);
}

static void on_usb_event(struct button *btn,
		const button_state_t event, const uint16_t clicks,
		const uint16_t repeats, void *ctx)
{
	unused(btn);
	unused(clicks);
	unused(repeats);
	struct usb_vbus *self = (struct usb_vbus *)ctx;
	process_event(event, self->cb, self->cb_ctx);
}

static void on_debug_button_event(struct button *btn,
		const button_state_t event, const uint16_t clicks,
		const uint16_t repeats, void *ctx)
{
	unused(btn);

	struct debug_button *self = (struct debug_button *)ctx;

	switch (event) {
	case BUTTON_STATE_PRESSED:
		debug("pressed: %d click(s) %d repeat(s)", clicks, repeats);
		break;
	case BUTTON_STATE_RELEASED:
		debug("released: %d click(s) %d repeat(s)", clicks, repeats);
		break;
	case BUTTON_STATE_HOLDING:
		debug("holding: %d click(s) %d repeat(s)", clicks, repeats);
		break;
	default:
		break;
	}

	process_event(event, self->cb, self->cb_ctx);
}

static button_level_t get_gpio_state(void *ctx)
{
	struct lm_gpio *gpio = (struct lm_gpio *)ctx;
	int level = lm_gpio_get(gpio);
	return level? BUTTON_LEVEL_LOW : BUTTON_LEVEL_HIGH;
}

static button_level_t get_estop_state(void *ctx)
{
	static button_level_t prev;

	usrinp_get_state_t get_state = (usrinp_get_state_t)(uintptr_t)ctx;
	int rc = get_state(USRINP_EMERGENCY_STOP);

	if (rc >= 0) {
		prev = rc == 0? BUTTON_LEVEL_LOW : BUTTON_LEVEL_HIGH;
	}

	return prev;
}

static button_level_t get_usb_vbus_state(void *ctx)
{
	static button_level_t prev;

	usrinp_get_state_t get_state = (usrinp_get_state_t)(uintptr_t)ctx;
	int rc = get_state(USRINP_USB_CONNECT);

	if (rc >= 0) {
		prev = rc == 0? BUTTON_LEVEL_LOW : BUTTON_LEVEL_HIGH;
	}

	return prev;
}

static bool process_button(struct debug_button *dbgbtn,
		const uint32_t time_ms, uint32_t *interval_ms)
{
	button_step(dbgbtn->button, time_ms);
	*interval_ms = dbgbtn->param.sampling_period_ms;

	bool busy = button_busy(dbgbtn->button);

	if (!busy && dbgbtn->raised) {
		dbgbtn->raised = false;
		lm_gpio_enable_interrupt(dbgbtn->gpio);
	}

	return busy;
}

static bool process_estop(struct emergency_stop *estop,
		const uint32_t time_ms, uint32_t *interval_ms)
{
	button_step(estop->button, time_ms);
	*interval_ms = estop->param.sampling_period_ms;

	return button_busy(estop->button);
}

static bool process_usb(struct usb_vbus *usb,
		const uint32_t time_ms, uint32_t *interval_ms)
{
	button_step(usb->button, time_ms);
	*interval_ms = usb->param.sampling_period_ms;

	return button_busy(usb->button);
}

static void process(struct usrinp *self)
{
	uint32_t next_period;

	do {
		uint32_t time_ms = board_get_time_since_boot_ms();
		uint32_t interval_ms;
		next_period = 0;

		if (process_button(&self->dbgbtn, time_ms, &interval_ms)) {
			next_period = interval_ms;
		}
		if (process_estop(&self->estop, time_ms, &interval_ms)) {
			next_period = next_period?
				MIN(interval_ms, next_period) : interval_ms;
		}
		if (process_usb(&self->usb, time_ms, &interval_ms)) {
			next_period = next_period?
				MIN(interval_ms, next_period) : interval_ms;
		}

		sleep_ms(next_period);
	} while (next_period);
}

static void *task(void *e)
{
	struct usrinp *self = (struct usrinp *)e;

	while (1) {
		sem_wait(&self->poll);

		process(self);

		metrics_set(UsrInpStackHighWatermark, METRICS_VALUE(
				board_get_current_stack_watermark()));
	}

	return 0;
}

static void initialize_dbgbtn(struct debug_button *dbgbtn, struct lm_gpio *gpio)
{
	dbgbtn->gpio = gpio;
	dbgbtn->param = (struct button_param) {
		.sampling_period_ms = 10,
		.debounce_duration_ms = 20,
		.repeat_delay_ms = 300,
		.repeat_rate_ms = 200,
		.click_window_ms = 500,
		.sampling_timeout_ms = 1000,
	};
	dbgbtn->button = button_new(get_gpio_state,
			dbgbtn->gpio, on_debug_button_event, dbgbtn);
	button_set_param(dbgbtn->button, &dbgbtn->param);
	button_enable(dbgbtn->button);

	lm_gpio_register_callback(dbgbtn->gpio, on_gpio_intr, &usrinp);
	lm_gpio_enable(dbgbtn->gpio);
}

static void initialize_estop(struct emergency_stop *estop,
		usrinp_get_state_t f_get_state)
{
	estop->param = (struct button_param) {
		.sampling_period_ms = 500,
		.debounce_duration_ms = 20,
		.sampling_timeout_ms = 1000,
	};
	estop->button = button_new(get_estop_state, (void *)(uintptr_t)
			f_get_state, on_emergency_stop, estop);
	button_set_param(estop->button, &estop->param);
	button_enable(estop->button);
}

static void initialize_usb(struct usb_vbus *usb, usrinp_get_state_t f_get_state)
{
	usb->param = (struct button_param) {
		.sampling_period_ms = 30,
		.debounce_duration_ms = 150,
		.sampling_timeout_ms = 1000,
	};
	usb->button = button_new(get_usb_vbus_state,
			(void *)(uintptr_t)f_get_state, on_usb_event, usb);
	button_set_param(usb->button, &usb->param);
	button_enable(usb->button);
}

void usrinp_register_event_cb(usrinp_t type, usrinp_event_cb_t cb, void *cb_ctx)
{
	switch (type) {
	case USRINP_DEBUG_BUTTON:
		usrinp.dbgbtn.cb = cb;
		usrinp.dbgbtn.cb_ctx = cb_ctx;
		break;
	case USRINP_EMERGENCY_STOP:
		usrinp.estop.cb = cb;
		usrinp.estop.cb_ctx = cb_ctx;
		break;
	case USRINP_USB_CONNECT:
		usrinp.usb.cb = cb;
		usrinp.usb.cb_ctx = cb_ctx;
		break;
	default:
		break;
	}
}

int usrinp_raise(usrinp_t type)
{
	if (type == USRINP_EMERGENCY_STOP) {
		usrinp.estop.raised = true;
	} else if (type == USRINP_USB_CONNECT) {
		usrinp.usb.raised = true;
	}

	return sem_post(&usrinp.poll);
}

int usrinp_init(struct lm_gpio *debug_button, usrinp_get_state_t f_get_state)
{
	initialize_dbgbtn(&usrinp.dbgbtn, debug_button);
	initialize_estop(&usrinp.estop, f_get_state);
	initialize_usb(&usrinp.usb, f_get_state);

	usrinp.get_state = f_get_state;

	sem_init(&usrinp.poll, 0, 0);

	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, STACK_SIZE_BYTES);

	return pthread_create(&usrinp.thread, &attr, task, &usrinp);
}
