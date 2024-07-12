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

#ifndef EXIO_H
#define EXIO_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>

typedef enum {
	EXIO_INTR_NONE		= 0x00,
	EXIO_INTR_EMERGENCY	= 0x01,
	EXIO_INTR_USB_CONNECT	= 0x02,
	EXIO_INTR_ACCELEROMETER	= 0x04,
} exio_intr_t;

int exio_init(void *i2c, void *gpio_reset);
exio_intr_t exio_get_intr_source(void);
int exio_get_intr_level(exio_intr_t intr);
int exio_set_metering_power(bool on);
int exio_set_sensor_power(bool on);
/**
 * @brief Set the power state of the audio device.
 *
 * This function controls the power state of the audio device connected
 * to the external IO. It uses the TCA9539 IO expander to control the
 * power supply to the audio device.
 *
 * @param[in] on If true, the audio device is powered on. If false, the audio
 * device is powered off.
 *
 * @return 0 if the operation was successful, or a negative error code if it
 * failed.
 *
 * @note It takes 120ms to power on the audio device and 80ms to power off.
 */
int exio_set_audio_power(bool on);
int exio_set_qca7005_reset(bool on);
int exio_set_w5500_reset(bool on);
int exio_set_led(bool on);

#if defined(__cplusplus)
}
#endif

#endif /* EXIO_H */
