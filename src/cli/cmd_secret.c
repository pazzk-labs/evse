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

#include "libmcu/pki.h"
#include "secret.h"

#define ECBUF_MAXLEN		512

static void println(const struct cli_io *io, const char *str)
{
	io->write(str, strlen(str));
	io->write("\n", 1);
}

static void process_dfu(int argc, const char *argv[], const struct cli_io *io)
{
	if (argc < 3) {
		println(io, "Usage: sec dfu [image|sign]\n");
		return;
	}

	if (strcmp(argv[2], "image") == 0) {
		println(io, "Update image key...");
	} else {
		println(io, "Unknown command");
	}
}

static void generate_x509_key(const struct cli_io *io)
{
	uint8_t key[ECBUF_MAXLEN];

	if (pki_generate_prikey(key, sizeof(key)) != 0) {
		println(io, "Failed to generate a private key");
		return;
	}

	if (secret_save(SECRET_KEY_X509_KEY, key, strlen((char *)key)+1) < 0) {
		println(io, "Failed to save the private key");
		return;
	}

	println(io, "Private key newly generated");
}

static void generate_x509_csr(const struct cli_io *io, const char *cn,
		const char *country, const char *org, const char *email)
{
	uint8_t key[ECBUF_MAXLEN];
	uint8_t csr[ECBUF_MAXLEN];

	if (secret_read(SECRET_KEY_X509_KEY, key, sizeof(key)) < 0) {
		println(io, "Failed to read the private key");
		return;
	}

	if (pki_generate_csr(csr, sizeof(csr), key, strlen((char *)key),
			&(struct pki_csr_params) {
				.common_name = cn,
				.country = country,
				.organization = org,
				.email = email,
			}) != 0) {
		println(io, "Failed to generate a CSR");
	}

	if (secret_save(SECRET_KEY_X509_KEY_CSR, csr,
			strlen((char *)csr)+1) < 0) {
		println(io, "Failed to save the CSR");
		return;
	}

	println(io, (char *)csr);
}

static void print_x509_key_csr(const struct cli_io *io)
{
	uint8_t csr[ECBUF_MAXLEN];
	if (secret_read(SECRET_KEY_X509_KEY_CSR, csr, sizeof(csr)) < 0) {
		println(io, "Failed to read the CSR");
		return;
	}
	println(io, (char *)csr);
}

DEFINE_CLI_CMD(sec, "sec {dfu|x509.key|x509.key.csr}") {
	struct cli const *cli = (struct cli const *)env;

	if (argc == 1) {
		return CLI_CMD_INVALID_PARAM;
	} else if (argc >= 2 && strcmp(argv[1], "dfu") == 0) {
		process_dfu(argc, argv, cli->io);
	} else if (argc == 2 && strcmp(argv[1], "x509.key") == 0) {
		generate_x509_key(cli->io);
	} else if (argc == 6 && strcmp(argv[1], "x509.key.csr") == 0) {
		generate_x509_csr(cli->io, argv[2], argv[3], argv[4], argv[5]);
	} else if (argc == 2 && strcmp(argv[1], "x509.key.csr") == 0) {
		print_x509_key_csr(cli->io);
	}

	return CLI_CMD_SUCCESS;
}
