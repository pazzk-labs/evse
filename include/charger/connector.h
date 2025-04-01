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

#ifndef CHARGER_CONNECTOR_H
#define CHARGER_CONNECTOR_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#include "libmcu/fsm.h"
#include "libmcu/ratelim.h"

#define CONNECTOR_LOG_RATE_CAP		10
#define CONNECTOR_LOG_RATE_SEC		2

#define CONNECTOR_EVENT_STRING_MAXLEN	128

typedef enum {
	E, /* initial state */
	A,
	B,
	C,
	D,
	F,
	Sn, /* state count */
} connector_state_t;

typedef enum {
	CONNECTOR_EVENT_NONE			= 0x00000000U,
	CONNECTOR_EVENT_PLUGGED			= 0x00000001U, /* EV connected */
	CONNECTOR_EVENT_UNPLUGGED		= 0x00000002U, /* EV disconnected */
	CONNECTOR_EVENT_CHARGING_STARTED	= 0x00000004U, /* C or D state */
	CONNECTOR_EVENT_CHARGING_SUSPENDED	= 0x00000008U, /* SuspendedEV */
	CONNECTOR_EVENT_CHARGING_ENDED		= 0x00000010U,
	CONNECTOR_EVENT_ERROR			= 0x00000020U,
	CONNECTOR_EVENT_ERROR_RECOVERY		= 0x00000040U,
	CONNECTOR_EVENT_BILLING_STARTED		= 0x00000080U, /* StartTransaction.conf */
	CONNECTOR_EVENT_BILLING_REALTIME	= 0x00000100U,
	CONNECTOR_EVENT_BILLING_ENDED		= 0x00000200U, /* StopTransaction.conf */
	CONNECTOR_EVENT_OCCUPIED		= 0x00000400U, /* occupied but not plugged yet */
	CONNECTOR_EVENT_UNOCCUPIED		= 0x00000800U, /* time out */
	CONNECTOR_EVENT_AUTH_REJECTED		= 0x00001000U,
	CONNECTOR_EVENT_RESERVED		= 0x00002000U,
	CONNECTOR_EVENT_ENABLED			= 0x00004000U,
} connector_event_t;

typedef enum {
	CONNECTOR_ERROR_NONE,
	CONNECTOR_ERROR_EV_SIDE,
	CONNECTOR_ERROR_EVSE_SIDE,
	CONNECTOR_ERROR_EMERGENCY_STOP,
} connector_error_t;

struct iec61851;
struct metering;
struct safety;

struct connector;

typedef void (*connector_event_cb_t)(struct connector *self,
		connector_event_t event, void *ctx);

struct connector_param {
	uint32_t max_output_current_mA; /* maximum output current in mA */
	uint32_t min_output_current_mA; /* minimum output current in mA */
	int16_t input_frequency; /* input frequency in Hz */

	struct iec61851 *iec61851;
	struct metering *metering;
	struct safety *safety;

	const char *name;
	int priority;
};

struct connector_api {
	int (*enable)(struct connector *self);
	int (*disable)(struct connector *self);
	int (*process)(struct connector *self);
};

struct charger;

struct connector {
	struct connector_param param;
	const struct connector_api *api;

	struct fsm fsm;

	connector_event_cb_t event_cb;
	void *event_cb_ctx;

	time_t time_last_state_change;
	connector_error_t error;

	struct ratelim log_ratelim; /* rate limiter for logging */

	int id;

	bool enabled;
	bool reserved;
};

/**
 * @brief Enable the connector.
 *
 * This function enables the connector, allowing it to operate.
 *
 * @param[in] self Pointer to the connector structure.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int connector_enable(struct connector *self);

/**
 * @brief Disable the connector.
 *
 * This function disables the connector, stopping its operation.
 *
 * @param[in] self Pointer to the connector structure.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int connector_disable(struct connector *self);

/**
 * @brief Process the connector state.
 *
 * This function processes the current state of the connector and performs
 * necessary actions.
 *
 * @param[in] self Pointer to the connector structure.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int connector_process(struct connector *self);

/**
 * @brief Register an event callback for the connector.
 *
 * This function registers a callback function that will be called when an event
 * occurs on the connector.
 *
 * @param[in] self Pointer to the connector structure.
 * @param[in] cb The callback function to register.
 * @param[in] cb_ctx User-defined context to pass to the callback function.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int connector_register_event_cb(struct connector *self,
		connector_event_cb_t cb, void *cb_ctx);

/**
 * @brief Get the current state of the connector.
 *
 * @param[in] self Pointer to the connector structure.
 *
 * @return The current state of the connector.
 */
connector_state_t connector_state(const struct connector *self);

/**
 * @brief Convert the connector state to a string representation.
 *
 * @param[in] state The connector state.
 *
 * @return The string representation of the state.
 */
const char *connector_stringify_state(const connector_state_t state);

/**
 * @brief Set the priority of the connector.
 *
 * @param[in] self Pointer to the connector structure.
 * @param[in] priority The priority to set.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int connector_set_priority(struct connector *self, const int priority);

/**
 * @brief Get the priority of the connector.
 *
 * @param[in] connector Pointer to the connector structure.
 *
 * @return The priority of the connector.
 */
int connector_priority(const struct connector *connector);

/**
 * @brief Get the name of the connector.
 *
 * This function returns the name of the connector as a string.
 *
 * @param[in] self Pointer to the connector structure.
 *
 * @return The name of the connector.
 */
const char *connector_name(const struct connector *self);

/**
 * @brief Get the metering information of the connector.
 *
 * Returns an instance of the meter module.
 *
 * @param[in] self Pointer to the connector structure.
 *
 * @return Pointer to the metering structure.
 */
struct metering *connector_meter(const struct connector *self);

/**
 * @brief Convert a connector event to a string representation.
 *
 * This function converts the given connector event to its string representation
 * and stores it in the provided buffer.
 *
 * @param[in] event The connector event to convert.
 * @param[out] buf The buffer to store the string representation.
 * @param[in] bufsize The size of the buffer.
 *
 * @return The number of characters written to the buffer, excluding the null
 *         terminator.
 */
size_t connector_stringify_event(const connector_event_t event,
		char *buf, size_t bufsize);

/**
 * @brief Check if the connector is enabled.
 *
 * @param[in] self Pointer to the connector structure.
 *
 * @return true if the connector is enabled, false otherwise.
 */
bool connector_is_enabled(const struct connector *self);

/**
 * @brief Check if the connector is reserved.
 *
 * @note The reserved state is added to handle cases where the connector is not
 * plugged in but has been reserved or preempted by user authentication through
 * higher-level protocols such as OCPP.
 *
 * @param[in] self Pointer to the connector structure.
 *
 * @return true if the connector is reserved, false otherwise.
 */
bool connector_is_reserved(const struct connector *self);

#if defined(__cplusplus)
}
#endif

#endif /* CHARGER_CONNECTOR_H */
