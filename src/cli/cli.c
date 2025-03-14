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

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include "libmcu/timext.h"

#define STACK_SIZE_BYTES	5376U
#define INPUT_SCAN_INTERVAL_MS	5

static int do_read(void *buf, size_t bufsize)
{
	return (int)fread(buf, bufsize, 1, stdin);
}

static int do_write(const void *data, size_t datasize)
{
	int rc = (int)fwrite(data, datasize, 1, stdout);

	if (rc >= 0 && datasize > 0) {
		fflush(stdout);
		fsync(fileno(stdout));
	}

	return rc;
}

static void *cli_task(void *e)
{
	struct cli *cli = (struct cli *)e;

	while (1) {
		cli_step(cli);
		sleep_ms(INPUT_SCAN_INTERVAL_MS);
	}

	return 0;
}

void cli_start(struct cli *cli)
{
	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, STACK_SIZE_BYTES);
	pthread_create(&thread, &attr, cli_task, cli);
}

const struct cli_io *cli_io_create(void)
{
	static const struct cli_io io = {
		.read = do_read,
		.write = do_write,
	};

	/* Disable buffering on stdin */
	setvbuf(stdin, NULL, _IONBF, 0);
	/* Enable non-blocking mode on stdin and stdout */
	fcntl(fileno(stdout), F_SETFL, 0);
	fcntl(fileno(stdin), F_SETFL, 0);

	return &io;
}
