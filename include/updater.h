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

#ifndef UPDATER_H
#define UPDATER_H

#if defined(__cplusplus)
extern "C" {
#endif

#if !defined(UPDATER_BUFFER_SIZE)
#define UPDATER_BUFFER_SIZE			1024
#endif
#if !defined(UPDATER_TIMEOUT_SEC)
#define UPDATER_TIMEOUT_SEC			(5/*min*/ * 60)
#endif
#if !defined(UPDATER_TRANSFER_TIMEOUT_MS)
#define UPDATER_TRANSFER_TIMEOUT_MS		10000
#endif

#if !defined(URL_MAXLEN)
#define URL_MAXLEN				256
#endif

#include <time.h>
#include <stdbool.h>
#include "downloader.h"

typedef enum {
	UPDATER_EVT_NONE,
	UPDATER_EVT_ERROR_PREPARING,
	UPDATER_EVT_START_DOWNLOADING,
	UPDATER_EVT_DOWNLOADING,
	UPDATER_EVT_ERROR_DOWNLOADING,
	UPDATER_EVT_ERROR_WRITING,
	UPDATER_EVT_ERROR_TIMEOUT,
	UPDATER_EVT_DOWNLOADED,
	UPDATER_EVT_START_INSTALLING,
	UPDATER_EVT_ERROR_INSTALLING,
	UPDATER_EVT_REBOOT_REQUIRED,
	UPDATER_EVT_INSTALLED,
	UPDATER_EVT_ERROR,
} updater_event_t;

typedef void (*updater_event_callback_t)(updater_event_t event, void *ctx);
typedef void (*updater_runner_t)(void *ctx);

struct updater_param {
	char url[URL_MAXLEN];
	time_t retrieve_date;
	int max_retries;
	int retry_interval;
};

/**
 * @brief Initializes the updater module.
 *
 * This function initializes the updater module and prepares it for use.
 *
 * @return An integer indicating the status of the operation. A return value
 *         of 0 indicates success, while any other value indicates an error.
 */
int updater_init(void);

/**
 * @brief Deinitializes the updater module.
 *
 * This function deinitializes the updater module and releases any resources
 * that were allocated during initialization.
 *
 * @return An integer indicating the status of the operation. A return value
 *         of 0 indicates success, while any other value indicates an error.
 */
int updater_deinit(void);

/**
 * @brief Registers an event callback for the updater module.
 *
 * This function registers a callback function that will be called when an
 * event occurs in the updater module.
 *
 * @param[in] cb The callback function to register.
 * @param[in] cb_ctx A pointer to the context that will be passed to the
 *            callback function when it is called.
 *
 * @return An integer indicating the status of the operation. A return value
 *         of 0 indicates success, while any other value indicates an error.
 */
int updater_register_event_callback(updater_event_callback_t cb, void *cb_ctx);

/**
 * @brief Sets the runner for the updater module.
 *
 * This function sets a runner for the updater module. The runner is a
 * function that will be called when an update request occurs. The context
 * pointer will be passed to the runner function when it is called.
 *
 * @param[in] runner The runner function to set.
 * @param[in] ctx A pointer to the context that will be passed to the runner
 *             function when it is called.
 *
 * @return An integer indicating the status of the operation. A return value
 *         of 0 indicates success, while any other value indicates an error.
 */
int updater_set_runner(updater_runner_t runner, void *ctx);

/**
 * @brief Requests an update using the specified parameters.
 *
 * This function requests an update using the parameters provided in the
 * updater_param structure and the specified downloader.
 *
 * @note If there is an existing update request, it will drop the new request if
 * the update has already started. However, if the existing request is in a
 * pending state (i.e., the retrieve date has not yet been arrived), it will
 * replace the pending request with the new one.
 *
 * @param[in] param A pointer to the updater_param structure containing the
 *             parameters for the update request.
 * @param[in] downloader A pointer to the downloader to use for the update.
 *
 * @return An integer indicating the status of the operation. A return value
 *         of 0 indicates success, while any other value indicates an error.
 */
int updater_request(const struct updater_param *param,
		struct downloader *downloader);

/**
 * @brief Processes the updater module.
 *
 * This function processes any pending operations in the updater module.
 *
 * @return An integer indicating the status of the operation. A return value
 *         of 0 indicates success, while any other value indicates an error.
 */
int updater_process(void);

/**
 * @brief Checks if the updater module is busy.
 *
 * This function checks if the updater module is currently busy with an
 * operation.
 *
 * @return A boolean value indicating whether the updater module is busy.
 *         A return value of true indicates that the module is busy, while
 *         false indicates that it is not.
 */
bool updater_busy(void);

#if defined(__cplusplus)
}
#endif

#endif /* UPDATER_H */
