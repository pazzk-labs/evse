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

#ifndef PING_H
#define PING_H

#if defined(__cplusplus)
extern "C" {
#endif

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
int ping_measure(const char *ipstr, const int timeout_ms);

#if defined(__cplusplus)
}
#endif

#endif /* PING_H */
