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

#include "logger.h"
#include <stdio.h>

static void logger_helper(logging_t type, const char *format, va_list args)
{
	char buf[256];
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
	vsnprintf(buf, sizeof(buf), format, args);
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#endif

	switch (type) {
	case LOGGING_TYPE_DEBUG:
		debug("%s", buf);
		break;
	case LOGGING_TYPE_INFO:
		info("%s", buf);
		break;
	case LOGGING_TYPE_WARN:
		warn("%s", buf);
		break;
	case LOGGING_TYPE_ERROR:
		error("%s", buf);
		break;
	default:
		break;
	}
}

void logger_debug(const char *format, va_list args)
{
	logger_helper(LOGGING_TYPE_DEBUG, format, args);
}

void logger_info(const char *format, va_list args)
{
	logger_helper(LOGGING_TYPE_INFO, format, args);
}

void logger_warn(const char *format, va_list args)
{
	logger_helper(LOGGING_TYPE_WARN, format, args);
}

void logger_error(const char *format, va_list args)
{
	logger_helper(LOGGING_TYPE_ERROR, format, args);
}
