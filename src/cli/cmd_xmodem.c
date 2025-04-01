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
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "libmcu/xmodem.h"
#include "libmcu/board.h"
#include "libmcu/timext.h"
#include "libmcu/dfu.h"
#include "libmcu/compiler.h"

#include "fs/fs.h"

#define DATA_BLOCK_SIZE			1024U
#define DOWNLOAD_TIMEOUT_MS		300000U
#define STRBUF_MAXLEN			256U

struct download_ctx {
	struct fs *fs;
	const char *filename;
	const struct cli_io *io;
	size_t filesize;
	uint32_t t0, t1;
	size_t offset;
	void *arg;
};

static const struct cli_io *tio;

static void println(const struct cli_io *io, const char *str)
{
	io->write(str, strlen(str));
	io->write("\n", 1);
}

static int input(uint8_t *buf, size_t bufsize, uint32_t timeout_ms)
{
	(void)timeout_ms;
	int rc = tio->read(buf, bufsize);
	if (rc <= 0) {
		sleep_ms(1); /* give some time for watchdog feed */
	}
	return rc;
}

static int output(const uint8_t *data, size_t datasize, uint32_t timeout_ms)
{
	(void)timeout_ms;
	return tio->write(data, datasize);
}

static void flush_input(void)
{
	sleep_ms(500); /* flush after whole fault packet is received */
	fflush(stdin);
}

static xmodem_error_t on_recv_dummy(size_t seq,
		const uint8_t *data, size_t datasize, void *ctx)
{
	unused(data);

	struct download_ctx *e = (struct download_ctx *)ctx;
	e->filesize += datasize;

	if (seq == 0) { /* last packet, no payload */
		e->t1 = board_get_time_since_boot_ms();
	} else if (seq == 1) { /* first packet */
		e->t0 = board_get_time_since_boot_ms();
	}

	return XMODEM_ERROR_NONE;
}

static xmodem_error_t download(xmodem_recv_callback_t cb, void *cb_ctx)
{
	uint8_t *buf = (uint8_t *)malloc(DATA_BLOCK_SIZE);
	if (buf == NULL) {
		return XMODEM_ERROR_NO_ENOUGH_BUFFER;
	}

	xmodem_set_rx_buffer(buf, DATA_BLOCK_SIZE);
	xmodem_set_millis((uint32_t(*)(void))board_get_time_since_boot_ms);
	xmodem_set_io(input, output, flush_input);

	xmodem_error_t err = xmodem_receive(XMODEM_DATA_BLOCK_1K,
			cb, cb_ctx, DOWNLOAD_TIMEOUT_MS);

	free(buf);

	return err;
}

static void test_download(const struct cli_io *io)
{
	struct download_ctx ctx = { 0, };
	tio = io;

	xmodem_error_t err = download(on_recv_dummy, &ctx);

	char str[STRBUF_MAXLEN];
	snprintf(str, sizeof(str), "Downloaded %zu bytes in %"PRIu32"ms(%"
			PRIu32"B/s): %d", ctx.filesize, ctx.t1 - ctx.t0,
			(uint32_t)(ctx.filesize*1000) / (ctx.t1 - ctx.t0), err);
	println(io, str);
}

static xmodem_error_t on_recv_firmware(size_t seq,
		const uint8_t *data, size_t datasize, void *ctx)
{
	struct download_ctx *e = (struct download_ctx *)ctx;
	struct dfu *dfu = (struct dfu *)e->arg;

	if (seq == 1) { /* first packet */
		e->t0 = board_get_time_since_boot_ms();

		const struct dfu_image_header *hdr =
			(const struct dfu_image_header *)data;
		const size_t offset = sizeof(*hdr);
		const size_t len = datasize - offset;

		if (!dfu_is_valid_header(hdr)
				|| dfu_prepare(dfu, hdr) != DFU_ERROR_NONE
				|| dfu_write(dfu, 0, &data[offset], len) < 0) {
			return XMODEM_ERROR_CANCELED;
		}
	} else if (seq == 0) { /* last packet */
		e->t1 = board_get_time_since_boot_ms();
	} else {
		if (dfu_write(dfu, 0, data, datasize) < 0) {
			return XMODEM_ERROR_CANCELED;
		}
	}

	return XMODEM_ERROR_NONE;
}

static void update_firmware(const struct cli_io *io)
{
	struct dfu *dfu = dfu_new(DATA_BLOCK_SIZE);
	struct download_ctx ctx = { .arg = dfu };

	tio = io;

	if (dfu == NULL) {
		return;
	}

	xmodem_error_t err = download(on_recv_firmware, &ctx);

	char str[STRBUF_MAXLEN];
	snprintf(str, sizeof(str), "Downloaded %zu bytes in %"PRIu32"ms(%"
			PRIu32"B/s): %d", ctx.filesize, ctx.t1 - ctx.t0,
			(uint32_t)(ctx.filesize*1000) / (ctx.t1 - ctx.t0), err);
	println(io, str);

	if (dfu_finish(dfu) == DFU_ERROR_NONE &&
			dfu_commit(dfu) == DFU_ERROR_NONE) {
		println(io, "Successfully updated firmware.");
	} else {
		println(io, "Failed to update firmware.");
	}

	dfu_delete(dfu);
}

DEFINE_CLI_CMD(xmodem, "XMODEM") {
	struct cli *cli = (struct cli *)env;

	if (argc == 2) {
		if (strcmp(argv[1], "test") == 0) {
			test_download(cli->io);
		} else if (strcmp(argv[1], "dfu") == 0) {
			update_firmware(cli->io);
		} else {
			return CLI_CMD_INVALID_PARAM;
		}
	}

	return CLI_CMD_SUCCESS;
}
