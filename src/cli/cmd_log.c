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

#include "libmcu/cli.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "app.h"
#include "logger.h"
#include "fs/logfs.h"

#if !defined(MIN)
#define MIN(a, b)		(((a) > (b))? (b) : (a))
#endif

#define MAX_BUFSIZE		4096U

static void println(const struct cli_io *io, const char *str)
{
	io->write(str, strlen(str));
	io->write("\n", 1);
}

static void on_dir(struct logfs *fs, const time_t ts, void *ctx)
{
	struct cli *cli = (struct cli *)ctx;
	char buf[32 + FS_FILENAME_MAX + 1];
	const size_t filesize = logfs_size(fs, ts);
	snprintf(buf, sizeof(buf)-1, "%ld (%zu bytes)", (long)ts, filesize);

	println(cli->io, buf);
}

static void delete_log(struct logfs *fs, const char *filename, struct cli *cli)
{
	if (!fs || !filename) {
		return;
	}

	const time_t ts = strtoll(filename, NULL, 10);
	int err;

	if ((err = logfs_delete(fs, ts)) < 0) {
		if (err == -ENOENT) {
			println(cli->io, "File not found");
		} else {
			println(cli->io, "Failed to delete file");
		}
	}
}

static void print_log(struct logfs *fs, const char *filename, struct cli *cli)
{
	if (!fs || !filename) {
		return;
	}

	int len;
	size_t filesize;
	char *buf;
	const time_t ts = strtoll(filename, NULL, 10);

	if ((filesize = logfs_size(fs, ts)) == 0) {
		println(cli->io, "Failed to get file size");
		return;
	}

	const size_t blocksize = MIN(filesize, MAX_BUFSIZE);
	if ((buf = (char *)malloc(blocksize)) == NULL) {
		println(cli->io, "Failed to allocate memory");
		return;
	}

	for (size_t i = 0; i < filesize; i += (size_t)len) {
		len = logfs_read(fs, ts, i, buf, blocksize);
		if (len < 0) {
			println(cli->io, "Failed to read file");
			break;
		}
		cli->io->write(buf, (size_t)len);
	}

	free(buf);
}

static void set_writer(const char *writer, struct cli *cli)
{
	log_writer_t writer_type = LOG_WRITER_NONE;

	if (!writer) {
		return;
	}

	if (strcmp(writer, "console") == 0) {
		writer_type = LOG_WRITER_CONSOLE;
	} else if (strcmp(writer, "file") == 0) {
		writer_type = LOG_WRITER_FILE;
	} else if (strcmp(writer, "all") == 0) {
		writer_type = LOG_WRITER_ALL;
	} else if (strcmp(writer, "none") == 0) {
	} else {
		println(cli->io, "Invalid writer");
		return;
	}

	logger_set_writer(writer_type);
}

static void set_level(const char *level, struct cli *cli)
{
	logging_t level_type = LOGGING_TYPE_DEBUG;

	if (!level) {
		return;
	}

	if (strcmp(level, "debug") == 0) {
	} else if (strcmp(level, "info") == 0) {
		level_type = LOGGING_TYPE_INFO;
	} else if (strcmp(level, "error") == 0) {
		level_type = LOGGING_TYPE_ERROR;
	} else if (strcmp(level, "none") == 0) {
		level_type = LOGGING_TYPE_NONE;
	} else {
		println(cli->io, "Invalid level");
		return;
	}

	logger_set_level(level_type);
}

static void print_logger_info(struct cli *cli)
{
	const logging_t level = logger_get_level();
	const log_writer_t writer = logger_get_writer();

	char buf[128];
	sprintf(buf, "logging to %s at %s level",
			writer == LOG_WRITER_ALL? "console & file" :
			writer == LOG_WRITER_CONSOLE? "console" :
			writer == LOG_WRITER_FILE? "file" : "none",
			level == LOGGING_TYPE_DEBUG? "debug" :
			level == LOGGING_TYPE_INFO? "info" :
			level == LOGGING_TYPE_ERROR? "error" : "none");
	println(cli->io, buf);
}

DEFINE_CLI_CMD(log, "Display recent system logs") {
	struct cli *cli = (struct cli *)env;
	struct app *app = (struct app *)cli->env;

	if (argc >= 2) {
		if (strcmp(argv[1], "clear") == 0) {
			logfs_clear(app->logfs);
		} else if (strcmp(argv[1], "rm") == 0) {
			delete_log(app->logfs, argc >= 3? argv[2] : NULL, cli);
		} else if (strcmp(argv[1], "show") == 0) {
			print_log(app->logfs, argc >= 3? argv[2] : NULL, cli);
		} else if (strcmp(argv[1], "set") == 0) {
			set_writer(argc >= 3? argv[2] : NULL, cli);
		} else if (strcmp(argv[1], "level") == 0) {
			set_level(argc >= 3? argv[2] : NULL, cli);
		} else if (strcmp(argv[1], "flush") == 0) {
			logfs_flush(app->logfs);
		} else {
			return CLI_CMD_INVALID_PARAM;
		}
		return CLI_CMD_SUCCESS;
	} else if (argc != 1) {
		return CLI_CMD_INVALID_PARAM;
	}

	print_logger_info(cli);
	logfs_dir(app->logfs, on_dir, cli, 0);

	return CLI_CMD_SUCCESS;
}
