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

#ifndef LOGGER_H
#define LOGGER_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdarg.h>
#include "libmcu/logging.h"
#include "fs/logfs.h"

#if !defined(LOGGER_FS_BASE_PATH)
#define LOGGER_FS_BASE_PATH	"logfs"
#endif
#if !defined(LOGGER_FS_MAX_SIZE)
#define LOGGER_FS_MAX_SIZE	(2 * 1024 * 1024) /* 2MiB */
#endif
#if !defined(LOGGER_FS_MAX_LOGS)
#define LOGGER_FS_MAX_LOGS	30 /* 30 days */
#endif
#if !defined(LOGGER_FS_CACHE_SIZE)
#define LOGGER_FS_CACHE_SIZE	4096 /* filesystem block size for performance */
#endif

enum {
	LOG_WRITER_NONE		= 0x00,
	LOG_WRITER_CONSOLE	= 0x01,
	LOG_WRITER_FILE		= 0x02,
	LOG_WRITER_ALL		= LOG_WRITER_CONSOLE | LOG_WRITER_FILE,
};

typedef uint8_t log_writer_t;

/**
 * @brief Initialize the logger with the given file system.
 *
 * This function sets up the logger to use the specified file system
 * for logging.
 *
 * @param[in] fs Pointer to the file system structure to be used for logging.
 */
void logger_init(struct logfs *fs);

/**
 * @brief Set the log writer function.
 *
 * This function sets the function that will be used to write log messages.
 *
 * @param[in] writer The log writer function to be used.
 */
void logger_set_writer(log_writer_t writer);

/**
 * @brief Set the logging level.
 *
 * This function sets the minimum level of messages that will be logged.
 *
 * @param[in] level The logging level to be set.
 */
void logger_set_level(logging_t level);

/**
 * @brief Get the current logging level.
 *
 * This function returns the current minimum level of messages that
 * will be logged.
 *
 * @return The current logging level.
 */
logging_t logger_get_level(void);

/**
 * @brief Get the current log writer function.
 *
 * This function returns the function that is currently being used to
 * write log messages.
 *
 * @return The current log writer function.
 */
log_writer_t logger_get_writer(void);

/**
 * @brief Log a debug message.
 *
 * This helper function logs a debug message using the specified format and
 * arguments.
 *
 * @param[in] format Format string for the log message.
 * @param[in] args Arguments for the format string.
 */
void logger_debug(const char *format, va_list args);

/**
 * @brief Log an informational message.
 *
 * This helper function logs an informational message using the specified format
 * and arguments.
 *
 * @param[in] format Format string for the log message.
 * @param[in] args Arguments for the format string.
 */
void logger_info(const char *format, va_list args);

/**
 * @brief Log a warning message.
 *
 * This helper function logs a warning message using the specified format and
 * arguments.
 *
 * @param[in] format Format string for the log message.
 * @param[in] args Arguments for the format string.
 */
void logger_warn(const char *format, va_list args);

/**
 * @brief Log an error message.
 *
 * This helper function logs an error message using the specified format and
 * arguments.
 *
 * @param[in] format Format string for the log message.
 * @param[in] args Arguments for the format string.
 */
void logger_error(const char *format, va_list args);

#if defined(__cplusplus)
}
#endif

#endif /* LOGGER_H */
