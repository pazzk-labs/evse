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
#include <inttypes.h>
#include "libmcu/metrics.h"
#include "libmcu/metricfs.h"
#include "cbor/cbor.h"
#include "app.h"

#define COLLECTION_BUFSIZE		1024
#define MAX_CBOR_ITEMS			128

static void println(const struct cli_io *io, const char *str)
{
	io->write(str, strlen(str));
	io->write("\n", 1);
}

static void println_quote(const struct cli_io *io, const char *str)
{
	io->write("\n\"", 2);
	io->write(str, strlen(str));
	io->write("\"", 1);
	io->write("\n", 1);
}

static void print_metric(metric_key_t key, int32_t value, void *ctx)
{
	struct cli const *cli = (struct cli const *)ctx;
	const char *str = metrics_stringify_key(key);
	char buf[2/*prettier*/+12/*digits*/+1/*null*/] = { 0, };

	if (str == NULL) {
		return;
	}

	cli->io->write(str, strlen(str));
	snprintf(buf, sizeof(buf)-1, ": %" PRId32 "\n", value);
	cli->io->write(buf, strlen(buf));
}

static void convert(cbor_reader_t const *reader, cbor_item_t const *item,
		cbor_item_t const *parent, void *arg)
{
	static const char *keystr;
	struct cli const *cli = (struct cli const *)arg;
	int32_t i32;

	if (parent == NULL || parent->type != CBOR_ITEM_MAP) {
		return;
	}

	cbor_decode(reader, item, &i32, sizeof(i32));

	if ((item - parent) % 2) { /* key */
		keystr = metrics_stringify_key((metric_key_t)i32);
	} else { /* value */
		char buf[2/*prettier*/+12/*digits*/+1/*null*/] = { 0, };
		cli->io->write(keystr, strlen(keystr));
		snprintf(buf, sizeof(buf)-1, ": %" PRId32 "\n", i32);
		cli->io->write(buf, strlen(buf));
	}
}

static void read_metrics(const metricfs_id_t id,
		const void *data, const size_t datasize, void *ctx)
{
	struct cli const *cli = (struct cli const *)ctx;
	cbor_reader_t reader;
	cbor_item_t items[MAX_CBOR_ITEMS];
	size_t n;

	char idstr[METRICFS_ID_MAXLEN+1];
	snprintf(idstr, sizeof(idstr)-1, "%u", id);
	println_quote(cli->io, idstr);

	cbor_reader_init(&reader, items, sizeof(items) / sizeof(items[0]));
	cbor_error_t err = cbor_parse(&reader, data, datasize, &n);

	if (err == CBOR_SUCCESS || err == CBOR_BREAK) {
		cbor_iterate(&reader, 0, convert, ctx);
	}
}

DEFINE_CLI_CMD(metric, "Show metrics") {
	struct cli *cli = (struct cli *)env;
	struct app *app = (struct app *)cli->env;

	if (argc == 2) {
		if (strcmp(argv[1], "reset") == 0) {
			metricfs_clear(app->mfs);
			metrics_reset();
		} else if (strcmp(argv[1], "show") == 0) {
			uint8_t buf[COLLECTION_BUFSIZE];
			metricfs_iterate(app->mfs, read_metrics, cli,
					buf, sizeof(buf), 0);
		}
		return CLI_CMD_SUCCESS;
	} else if (argc != 1) {
		return CLI_CMD_INVALID_PARAM;
	}

	char buf[64];
	snprintf(buf, sizeof(buf)-1, "Out of %zu metrics:", metrics_count());
	println(cli->io, buf);

	metrics_iterate(print_metric, cli);

	println(cli->io, "");
	snprintf(buf, sizeof(buf)-1, "Saved metrics collections: %" PRIu16,
			metricfs_count(app->mfs));
	println(cli->io, buf);

	return CLI_CMD_SUCCESS;
}
