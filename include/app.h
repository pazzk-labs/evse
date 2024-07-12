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

#ifndef APP_H
#define APP_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <time.h>

#include "pinmap.h"
#include "pilot.h"

struct kvstore;
struct logfs;
struct metricfs;

struct charger;
struct relay;

#define SYSTEM_TIME_MAX_DRIFT_SEC		(5)

typedef void (*app_metrics_cb_t)(void *ctx, const char *filepath,
		const void *data, const size_t datasize);

struct app {
	struct pinmap_periph periph;
	struct relay *relay;
	struct pilot *pilot;

	struct fs *fs;
	struct kvstore *kvstore;
	struct metricfs *mfs;
	struct logfs *logfs;

	struct {
		/* NOTE: do not use heap memory allocation for these buffers
		 * as it is used in DMA by ADC driver. In ESP32, DMA buffer
		 * should be allocated from internal memory. */
		struct {
			uint16_t tx[PILOT_NUMBER_OF_SAMPLES];
			uint16_t rx[PILOT_NUMBER_OF_SAMPLES];
		} buffer;
	} adc;

	struct charger *charger;
};

/**
 * @brief Reboot the application.
 *
 * This function performs necessary cleanup tasks and saves essential
 * information before rebooting the application. It ensures that the system
 * is in a safe state before initiating the reboot process.
 *
 * @note It can take up to 2 seconds to reboot the system.
 */
void app_reboot(void);

/**
 * @brief Adjust the system time if the drift exceeds a specified threshold.
 *
 * This function checks the difference between the current system time and the
 * provided Unix time. If the difference exceeds the specified drift threshold,
 * the system time is updated to the provided Unix time.
 *
 * @param[in] unixtime The Unix time to compare against the current system time.
 * @param[in] drift The maximum allowed drift in seconds. If the difference
 *            between the current system time and the provided Unix time exceeds
 *            this value, the system time will be updated.
 */
void app_adjust_time_on_drift(const time_t unixtime, const uint32_t drift);

/**
 * @brief Initialize the application.
 *
 * This function sets up the necessary components and configurations
 * for the application to run.
 */
void app_init(struct app *app);

/**
 * @brief Process the application logic.
 *
 * This function processes the main logic of the application and updates
 * the next period for the application's main loop.
 *
 * @param[out] next_period_ms Pointer to a variable where the next period in
 *                       milliseconds will be stored.
 * @return int Returns 0 on success, otherwise an error code.
 */
int app_process(uint32_t *next_period_ms);

#if defined(__cplusplus)
}
#endif

#endif /* APP_H */
