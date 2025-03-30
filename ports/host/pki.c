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

#include "libmcu/pki.h"
#include <errno.h>
#include "libmcu/compiler.h"

int pki_generate_prikey(uint8_t *key_buf, size_t key_bufsize)
{
	unused(key_buf);
	unused(key_bufsize);

	return -ENOTSUP;
}

int pki_generate_keypair(uint8_t *key_buf, size_t key_bufsize,
                         uint8_t *pub_buf, size_t pub_bufsize)
{
	unused(key_buf);
	unused(key_bufsize);
	unused(pub_buf);
	unused(pub_bufsize);

	return -ENOTSUP;
}

int pki_generate_csr(uint8_t *csr_buf, size_t csr_bufsize,
                     const uint8_t *prikey, size_t prikey_len,
                     const struct pki_csr_params *params)
{
	unused(csr_buf);
	unused(csr_bufsize);
	unused(prikey);
	unused(prikey_len);
	unused(params);

	return -ENOTSUP;
}

int pki_verify_cert(const uint8_t *cacert, size_t cacert_len,
                    const uint8_t *cert, size_t cert_len)
{
	unused(cacert);
	unused(cacert_len);
	unused(cert);
	unused(cert_len);

	return -ENOTSUP;
}
