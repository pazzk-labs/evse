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

#include "libmcu/gpio.h"
#include "libmcu/compiler.h"

#include <errno.h>

#include "driver/gpio.h"
#include "pinmap_gpio.h"

struct gpio {
	struct gpio_api api;

	uint16_t pin;
	gpio_callback_t callback;
	void *callback_ctx;

	int (*boot)(struct gpio *self);
};

static void on_gpio_interrupt(void *arg)
{
	struct gpio *gpio = (struct gpio *)arg;

        if (gpio->callback) {
		(*gpio->callback)(gpio, gpio->callback_ctx);
	}
}

static int set_input(struct gpio *self,
		const gpio_int_type_t intr, const gpio_pullup_t pullup)
{
	gpio_config_t cnf = {
		.intr_type = intr,
		.mode = GPIO_MODE_INPUT,
		.pin_bit_mask = (1ULL << self->pin),
		.pull_up_en = pullup,
	};

	esp_err_t err = gpio_config(&cnf);

	if (intr != GPIO_INTR_DISABLE) {
		err |= gpio_isr_handler_add(self->pin, on_gpio_interrupt, self);
	}

	return err;
}

static int set_input_int_anyedge(struct gpio *self)
{
	return set_input(self, GPIO_INTR_ANYEDGE, GPIO_PULLUP_DISABLE);
}

static int set_input_int_falling(struct gpio *self)
{
	return set_input(self, GPIO_INTR_NEGEDGE, GPIO_PULLUP_DISABLE);
}

static int set_input_int_falling_pullup(struct gpio *self)
{
	return set_input(self, GPIO_INTR_NEGEDGE, GPIO_PULLUP_ENABLE);
}

static int set_output_pullup(struct gpio *self)
{
	gpio_config_t cnf = {
		.intr_type = GPIO_INTR_DISABLE,
		.mode = GPIO_MODE_INPUT_OUTPUT,
		.pin_bit_mask = (1ULL << self->pin),
		.pull_up_en = true,
	};

	return gpio_config(&cnf);
}

static struct gpio gpio_tbl[] = {
	{ .pin = PINMAP_SPI2_CS_QCA7005, .boot = set_output_pullup, },
	{ .pin = PINMAP_SPI2_CS_W5500, .boot = set_output_pullup, },
	{ .pin = PINMAP_SPI3_CS_ADC, .boot = set_output_pullup, },
	{ .pin = PINMAP_OUTPUT_POWER, .boot = set_input_int_falling, },
	{ .pin = PINMAP_INPUT_POWER, .boot = set_input_int_falling, },
	{ .pin = PINMAP_IO_EXPANDER_RESET, .boot = set_output_pullup, },
	{ .pin = PINMAP_IO_EXPANDER_INT, .boot = set_input_int_anyedge, },
	{ .pin = PINMAP_W5500_INT, .boot = set_input_int_anyedge, },
	{ .pin = PINMAP_QCA7005_INT, .boot = set_input_int_anyedge, },
	{ .pin = PINMAP_METERING_INT, .boot = set_input_int_anyedge, },
	{ .pin = PINMAP_DEBUG_BUTTON, .boot = set_input_int_falling_pullup, },
};

static struct gpio *find_gpio_by_pin(uint16_t pin)
{
	size_t len = sizeof(gpio_tbl) / sizeof(gpio_tbl[0]);

	for (size_t i = 0; i < len; i++) {
		if (gpio_tbl[i].pin == pin) {
			return &gpio_tbl[i];
		}
	}

	return NULL;
}

static int enable_gpio(struct gpio *self)
{
	if (self->boot) {
		return self->boot(self);
	}

	return -ERANGE;
}

static int disable_gpio(struct gpio *self)
{
	unused(self);
	return 0;
}

static int enable_interrupt(struct gpio *self)
{
	return gpio_intr_enable(self->pin);
}

static int disable_interrupt(struct gpio *self)
{
	return gpio_intr_disable(self->pin);
}

static int set_gpio(struct gpio *self, int value)
{
	return gpio_set_level(self->pin, (uint32_t)value);
}

static int get_gpio(struct gpio *self)
{
	return gpio_get_level(self->pin);
}

static int register_callback(struct gpio *self,
		gpio_callback_t cb, void *cb_ctx)
{
	self->callback = cb;
	self->callback_ctx = cb_ctx;
	return 0;
}

struct gpio *gpio_create(uint16_t pin)
{
	struct gpio *p = find_gpio_by_pin(pin);

	if (p) {
		p->api = (struct gpio_api) {
			.enable = enable_gpio,
			.disable = disable_gpio,
			.enable_interrupt = enable_interrupt,
			.disable_interrupt = disable_interrupt,
			.set = set_gpio,
			.get = get_gpio,
			.register_callback = register_callback,
		};
	}

	return p;
}

void gpio_delete(struct gpio *self)
{
	unused(self);
}
