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
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>

#include "libmcu/board.h"
#include "libmcu/cleanup.h"
#include "fs/logfs.h"
#include "config.h"

struct logger {
	struct logfs *fs;

	struct {
		struct logging_backend console;
		struct logging_backend file;
		log_writer_t enabled;
	} writer;

	char scratch[LOGGING_MESSAGE_MAXLEN]; /* thread-safe as backend writer
						is called with mutex. */
	logging_t level;

	pthread_mutex_t file_mutex;
};

static struct logger m;

static void flush_logfs(void *arg)
{
	struct logfs *fs = (struct logfs *)arg;

	pthread_mutex_lock(&m.file_mutex);
	logfs_flush(fs);
	pthread_mutex_unlock(&m.file_mutex);
}

static size_t write_fs(const void *data, size_t size)
{
	const time_t sec = time(NULL);
	const time_t day = sec - (sec % (24 * 60 * 60));

	size_t len = logging_stringify(m.scratch, sizeof(m.scratch)-2, data);
	m.scratch[len++] = '\n';
	m.scratch[len] = '\0';

	pthread_mutex_lock(&m.file_mutex);
	int err = logfs_write(m.fs, day, m.scratch, len);
	pthread_mutex_unlock(&m.file_mutex);

	return err < 0? 0 : (size_t)(err);
}

static size_t write_stdout(const void *data, size_t size)
{
	size_t len = logging_stringify(m.scratch, sizeof(m.scratch)-2, data);

	m.scratch[len++] = '\n';
	m.scratch[len] = '\0';

	const size_t rc = fwrite(m.scratch, len, 1, stdout);

	return rc == 0? size : 0;
}

static void initialize_console_backend(struct logger *logger)
{
	logger->writer.console = (struct logging_backend) {
		.write = write_stdout,
	};

	logging_add_backend(&logger->writer.console);
}

static void initialize_file_backend(struct logger *logger)
{
	static bool cleanup_registered = false;

	logger->writer.file = (struct logging_backend) {
		.write = write_fs,
	};

	logging_add_backend(&logger->writer.file);

	if (!cleanup_registered) {
		cleanup_registered = true;
		cleanup_register(0, flush_logfs, logger->fs);
	}

	pthread_mutex_lock(&m.file_mutex);
	logfs_flush(logger->fs);
	pthread_mutex_unlock(&m.file_mutex);
}

static void set_level(struct logger *logger, logging_t level)
{
	if (level == logger->level) {
		return;
	}

	logger->level = level;
	logging_set_level_global(level);
	config_set("log.level", &level, sizeof(level));
}

static void set_writer(struct logger *logger, log_writer_t writer)
{
	if (writer == logger->writer.enabled) {
		return;
	}

	if (logger->writer.enabled & LOG_WRITER_CONSOLE &&
			!(writer & LOG_WRITER_CONSOLE)) {
		logging_remove_backend(&logger->writer.console);
	}
	if (logger->writer.enabled & LOG_WRITER_FILE &&
			!(writer & LOG_WRITER_FILE)) {
		logging_remove_backend(&logger->writer.file);
	}

	if (writer & LOG_WRITER_CONSOLE &&
			!(logger->writer.enabled & LOG_WRITER_CONSOLE)) {
		initialize_console_backend(logger);
	}
	if (writer & LOG_WRITER_FILE &&
			!(logger->writer.enabled & LOG_WRITER_FILE)) {
		initialize_file_backend(logger);
	}

	logger->writer.enabled = writer;
	config_set("log.mode", &writer, sizeof(writer));
}

void logger_set_level(logging_t level)
{
	set_level(&m, level);
}

void logger_set_writer(log_writer_t writer)
{
	set_writer(&m, writer);
}

logging_t logger_get_level(void)
{
	return m.level;
}

log_writer_t logger_get_writer(void)
{
	return m.writer.enabled;
}

void logger_init(struct logfs *fs)
{
	logging_t level = LOGGING_TYPE_DEBUG;
	log_writer_t writer = LOG_WRITER_ALL;

	config_get("log.mode", &writer, sizeof(writer));
	config_get("log.level", &level, sizeof(level));

	memset(&m, 0, sizeof(m));
	m.fs = (struct logfs *)fs;

	pthread_mutex_init(&m.file_mutex, NULL);
	logging_init(board_get_time_since_boot_ms);

	set_writer(&m, writer);
	set_level(&m, level);

	info("logging to %s at %s level",
			writer == LOG_WRITER_ALL? "console & file" :
			writer == LOG_WRITER_CONSOLE? "console" :
			writer == LOG_WRITER_FILE? "file" : "none",
			level == LOGGING_TYPE_DEBUG? "debug" :
			level == LOGGING_TYPE_INFO? "info" :
			level == LOGGING_TYPE_ERROR? "error" : "none");
}
