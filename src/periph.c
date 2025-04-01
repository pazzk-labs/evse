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

#include "periph.h"

#include <errno.h>

#include "libmcu/i2c.h"
#include "libmcu/gpio.h"
#include "libmcu/metrics.h"
#include "libmcu/actor.h"

#include "pinmap.h"
#include "exio.h"
#include "usrinp.h"

#include "logger.h"

static struct actor exio_actor;

static void exio_handler(struct actor *actor, struct actor_msg *msg)
{
	unused(actor);
	exio_intr_t intr = exio_get_intr_source();

	if (intr & EXIO_INTR_EMERGENCY) {
		usrinp_raise(USRINP_EMERGENCY_STOP);
	}
	if (intr & EXIO_INTR_USB_CONNECT) {
		usrinp_raise(USRINP_USB_CONNECT);
	}
	if (intr & EXIO_INTR_ACCELEROMETER) {
	}

	actor_free(msg);
	metrics_increase(ExioDispatchCount);
}

static void on_exio_intr(struct lm_gpio *gpio, void *ctx)
{
	unused(gpio);
	unused(ctx);
	actor_send(&exio_actor, NULL);
}

static int create_i2c_devices(struct pinmap_periph *p)
{
	int err = 0;

	err |= lm_i2c_enable(p->i2c0);
	err |= lm_i2c_reset(p->i2c0);

	p->io_expander = lm_i2c_create_device(p->i2c0, 0x74, 400000);
	p->codec = lm_i2c_create_device(p->i2c0, 0x18, 400000);
	p->temp = lm_i2c_create_device(p->i2c0, 0x49, 400000);
	p->acc = lm_i2c_create_device(p->i2c0, 0x19, 400000);

	if (!p->io_expander || !p->codec || !p->temp || !p->acc) {
		err |= -ENOMEM;
	}

	return err;
}

int periph_init(struct pinmap_periph *periph)
{
	int err = create_i2c_devices(periph);

	actor_set(&exio_actor, exio_handler, 0);

	err |= exio_init(periph->io_expander, periph->io_expander_reset);
	err |= exio_set_audio_power(false);

	err |= lm_gpio_register_callback(periph->io_expander_int,
			on_exio_intr, NULL);
	err |= lm_gpio_enable(periph->io_expander_int);

	if (err) {
		error("Failed to initialize peripherals: %d", err);
	}

	return err;
}
