/*
 * This file is part of the Pazzk project <https://pazzk.net/>.
 * Copyright (c) 2025 Pazzk <team@pazzk.net>.
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

#include "net/wifi_ap_info.h"
#include <errno.h>
#include "fs/fs.h"
#include "libmcu/compiler.h"

static const char *info_path = "wifi/ap_list";

int wifi_ap_info_add(struct fs *fs, const struct wifi_ap_info *ap_info)
{
	int err = fs_append(fs, info_path, ap_info, sizeof(*ap_info));
	return err >= 0 ? 0 : err;
}

int wifi_ap_info_del(struct fs *fs, const uint8_t bssid[WIFI_MAC_ADDR_LEN])
{
	unused(fs);
	unused(bssid);
	return -ENOTSUP;
}

int wifi_ap_info_del_by_ssid(struct fs *fs, const char *ssid)
{
	unused(fs);
	unused(ssid);
	return -ENOTSUP;
}

int wifi_ap_info_del_all(struct fs *fs)
{
	return fs_delete(fs, info_path);
}

int wifi_ap_info_get_all(struct fs *fs,
		struct wifi_ap_info *ap_info, int max_count)
{
	int i;

	for (i = 0; i < max_count; i++) {
		size_t offset = (size_t)i * sizeof(*ap_info);
		int err = fs_read(fs, info_path,
				offset, &ap_info[i], sizeof(*ap_info));
		if (err <= 0) {
			break;
		}
	}

	return i;
}
