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

#ifndef USER_INPUT_H
#define USER_INPUT_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "libmcu/gpio.h"

typedef enum {
	USRINP_DEBUG_BUTTON,      /**< Debug button input */
	USRINP_EMERGENCY_STOP,    /**< Emergency stop input */
	USRINP_USB_CONNECT,       /**< USB connection input */
} usrinp_t;

typedef enum {
	USRINP_EVENT_UNKNOWN,        /**< Unknown event */
	USRINP_EVENT_CONNECTED,      /**< Input connected (pressed) */
	USRINP_EVENT_DISCONNECTED,   /**< Input disconnected (released) */
} usrinp_event_t;

/**
 * @brief Function pointer type for getting the state of a user input.
 *
 * @param[in] source The user input source.
 *
 * @return The state of the user input.
 */
typedef int (*usrinp_get_state_t)(usrinp_t source);

/**
 * @brief Function pointer type for user input event callback.
 *
 * @param[in] event The user input event.
 * @param[in] ctx The context for the callback.
 */
typedef void (*usrinp_event_cb_t)(usrinp_event_t event, void *ctx);

/**
 * @brief Initialize user input handling.
 *
 * @param[in] debug_button Pointer to the GPIO structure for the debug button.
 * @param[in] f_get_state Function pointer to get the state of a user input.
 *
 * @return 0 on success, negative error code on failure.
 */
int usrinp_init(struct gpio *debug_button, usrinp_get_state_t f_get_state);

/**
 * @brief Raise a user input event.
 *
 * @param[in] type The type of user input.
 *
 * @return 0 on success, negative error code on failure.
 */
int usrinp_raise(usrinp_t type);

/**
 * @brief Register a callback for a user input event.
 *
 * @param[in] type The type of user input.
 * @param[in] cb The callback function to register.
 * @param[in] cb_ctx The context for the callback.
 */
void usrinp_register_event_cb(usrinp_t type,
		usrinp_event_cb_t cb, void *cb_ctx);

#if defined(__cplusplus)
}
#endif

#endif /* USER_INPUT_H */
