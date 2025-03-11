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

#ifndef VERSION_H
#define VERSION_H

#if defined(__cplusplus)
extern "C" {
#endif

#if !defined(MAKE_VERSION)
#define MAKE_VERSION(major, minor, patch)	\
	(((major) << 16) | ((minor) << 8) | (patch))
#endif

#if !defined(GET_VERSION_MAJOR)
#define GET_VERSION_MAJOR(version)		(((version) >> 16) & 0xff)
#endif
#if !defined(GET_VERSION_MINOR)
#define GET_VERSION_MINOR(version)		(((version) >> 8) & 0xff)
#endif
#if !defined(GET_VERSION_PATCH)
#define GET_VERSION_PATCH(version)		((version) & 0xff)
#endif

#define VERSION_MAJOR				0
#define VERSION_MINOR				0
#define VERSION_PATCH				1

#define VERSION					\
	MAKE_VERSION(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)

#if defined(__cplusplus)
}
#endif

#endif /* VERSION_H */
