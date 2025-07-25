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

#ifndef NET_H
#define NET_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "net/netif.h"
#include "net/ip.h"

#if !defined(NETMGR_CONNECT_TIMEOUT_MS)
#define NETMGR_CONNECT_TIMEOUT_MS		(10U * 1000)
#endif
#if !defined(NETMGR_MAX_RETRY)
#define NETMGR_MAX_RETRY			200U /* about 16 hours */
#endif
#if !defined(NETMGR_MAX_BACKOFF_MS)
#define NETMGR_MAX_BACKOFF_MS			(300U * 1000) /* 5min */
#endif

/**
 * @brief Enumeration of network manager events.
 *
 * This enumeration defines the possible events that can be raised by the
 * network manager.
 */
typedef enum {
	NETMGR_STATE_OFF			= 0x01U,
	NETMGR_STATE_EXHAUSTED			= 0x02U,
	NETMGR_STATE_INITIALIZED		= 0x04U,
	NETMGR_STATE_DISABLED			= 0x08U,
	NETMGR_STATE_ENABLED			= 0x10U,
	NETMGR_STATE_DISCONNECTED		= 0x20U,
	NETMGR_STATE_CONNECTED			= 0x40U,
	NETMGR_STATE_ANY			= 0x00U,
} netmgr_state_t;

typedef uint8_t netmgr_state_mask_t;

/**
 * @brief Callback type for network manager events.
 *
 * This type defines the function signature for callbacks that handle network
 * manager events.
 *
 * @param[in] event The network manager event that occurred.
 * @param[in] ctx A user-defined context pointer that is passed to the callback.
 */
typedef void (*netmgr_event_cb_t)(const netmgr_state_t event, void *ctx);

/**
 * @typedef netmgr_unrecoverable_cb_t
 * @brief Callback type for handling unrecoverable network errors.
 *
 * This callback is invoked when the network manager encounters an
 * unrecoverable error. It allows the application to perform custom
 * error handling or recovery actions.
 *
 * @param[in] ctx User-defined context passed to the callback.
 */
typedef void (*netmgr_unrecoverable_cb_t)(void *ctx);

/**
 * @brief Typedef for a network manager task function.
 *
 * This type defines a function pointer for a task that can be registered with
 * the network manager. The task function will be called with a context pointer
 * when the network is connected.
 *
 * If the function returns false, the task will be terminated. If the function
 * returns true, the task will continue to be called repeatedly.
 *
 * @param[in] ctx A pointer to the context for the task.
 *
 * @return A return value of true indicates that the task should continue to be
 *         called repeatedly, while false indicates that the task should be
 *         terminated.
 */
typedef bool (*netmgr_task_t)(void *ctx);

/**
 * @brief Initializes the network manager.
 *
 * This function initializes the network manager with the specified
 * health check interval. It sets up necessary configurations and
 * prepares the network manager for operation.
 *
 * @param[in] healthchk_interval_ms The interval in milliseconds for
 *                              performing network health checks.
 * @return int Returns 0 on success, or a negative error code on failure.
 */
int netmgr_init(uint32_t healthchk_interval_ms);

/**
 * @brief Deinitialize the network manager.
 *
 * This function deinitializes the network manager and releases any resources
 * it has allocated.
 */
void netmgr_deinit(void);

/**
 * @brief Enable the network manager.
 *
 * This function enables the network manager, allowing it to manage network
 * interfaces and connections.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int netmgr_enable(void);

/**
 * @brief Disable the network manager.
 *
 * This function disables the network manager, stopping it from managing
 * network interfaces and connections.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int netmgr_disable(void);

/**
 * @brief Registers a callback for unrecoverable network errors.
 *
 * This function allows the application to register a callback that will
 * be invoked when the network manager encounters an unrecoverable error.
 * The callback can be used to handle the error or perform recovery actions.
 *
 * @param[in] cb The callback function to handle unrecoverable errors.
 * @param[in] ctx User-defined context to pass to the callback.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int netmgr_register_unrecoverable_cb(netmgr_unrecoverable_cb_t cb, void *ctx);

/**
 * @brief Register a network interface with the network manager.
 *
 * This function registers a network interface with the network manager,
 * allowing it to be managed. The function takes the network interface, its
 * priority, MAC address, and IP information as parameters.
 *
 * @param[in] netif The network interface to register.
 * @param[in] priority The priority of the network interface.
 * @param[in] mac The MAC address of the network interface.
 * @param[in] static_ip The IP information of the network interface.
 *
 * @note If static_ip is NULL, the network manager will attempt to obtain
 *       an IP address using DHCP.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int netmgr_register_iface(struct netif *netif, const int priority,
		const uint8_t mac[NETIF_MAC_ADDR_LEN],
		const ip_info_t *static_ip);

/**
 * @brief Register a callback for network manager events.
 *
 * This function registers a callback function that will be called when
 * network manager events specified by the mask occur. The callback function
 * will be passed the event context.
 *
 * @param[in] mask The mask specifying which events to listen for.
 * @param[in] cb The callback function to register.
 * @param[in] ctx The context to pass to the callback function.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int netmgr_register_event_cb(const netmgr_state_mask_t mask,
		netmgr_event_cb_t cb, void *ctx);

/**
 * @brief Check if the network is connected.
 *
 * This function checks if the network is currently connected.
 *
 * @return true if the network is connected, false otherwise.
 */
bool netmgr_connected(void);

/**
 * @brief Measure the ping time to a specified IP address.
 *
 * This function sends a ping request to the specified IP address and waits
 * for a response within the given timeout period. It returns the round-trip
 * time in milliseconds.
 *
 * @param[in] ipstr The IP address of the target host as a string.
 * @param[in] timeout_ms The timeout period in milliseconds to wait for
 *            a response.
 *
 * @return The round-trip time in milliseconds on success, or a negative error
 *         code on failure.
 */
int netmgr_ping(const char *ipstr, const uint32_t timeout_ms);

/**
 * @brief Get the current state of the network manager.
 *
 * This function returns the current state of the network manager.
 *
 * @return The current state of the network manager as a netmgr_state_t value.
 */
netmgr_state_t netmgr_state(void);

/**
 * @brief Register a one-time task to be executed when the network is connected.
 *
 * This function registers a one-time task that will be executed when the
 * network is connected. The task will be called with the provided context.
 *
 * @param[in] task_func The task function to be executed.
 * @param[in] task_ctx The context to be passed to the task function.
 *
 * @return 0 on success, negative error code on failure.
 */
int netmgr_register_task(netmgr_task_t task_func, void *task_ctx);

#if defined(UNIT_TEST)
/**
 * @brief Perform a single step in the network manager state machine.
 *
 * This function advances the network manager state machine by one step,
 * transitioning to the next state based on the current state and the
 * defined state transitions.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int netmgr_step(void);
#endif

#if defined(__cplusplus)
}
#endif

#endif /* NET_H */
