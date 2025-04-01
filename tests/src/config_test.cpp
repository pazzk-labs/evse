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

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "config.h"
#include "libmcu/kvstore.h"
#include "libmcu/crc32.h"
#include "secret.h"

#define X509_BUFSIZE		2048

extern struct kvstore *mock_kvstore_create(void);
extern void mock_kvstore_destroy(struct kvstore *kvstore);

static void on_config_save(void *ctx) {
	mock().actualCall("on_config_save")
		.withPointerParameter("ctx", ctx);
}

int secret_read(secret_key_t key, void *buf, size_t bufsize) {
	return 0;
}

TEST_GROUP(Config) {
	struct kvstore *kvstore;
	struct config config;

	void setup(void) {
		config = (struct config) {
			.device_id = "my device",
		};

		kvstore = mock_kvstore_create();

		mock().expectOneCall("open")
			.withPointerParameter("self", kvstore)
			.withStringParameter("ns", "config")
			.andReturnValue(0);
		mock().expectOneCall("read")
			.withStringParameter("key", "basic")
			.withOutputParameterReturning("buf", &config, sizeof(config))
			.withParameter("size", sizeof(config))
			.andReturnValue(0);
		config_init(kvstore, on_config_save, NULL);
	}
	void teardown(void) {
		mock_kvstore_destroy(kvstore);

		mock().checkExpectations();
		mock().clear();
	}

	void go_default(void) {
		mock().expectOneCall("write")
			.withStringParameter("key", "basic")
			.withParameter("size", sizeof(config))
			.ignoreOtherParameters()
			.andReturnValue(0);
		mock().expectOneCall("on_config_save")
			.withPointerParameter("ctx", NULL);
		LONGS_EQUAL(0, config_save());
	}
};

TEST(Config, ShouldReturnDefaultConfig) {
	struct config actual = { 0, };
	config_get("version", &actual.version, sizeof(actual.version));
	LONGS_EQUAL(CONFIG_VERSION, actual.version);
	config_get("device.id", actual.device_id, sizeof(actual.device_id));
	STRCMP_EQUAL(config.device_id, actual.device_id);
	config_get("device.mode", &actual.device_mode, sizeof(actual.device_mode));
	LONGS_EQUAL(0, actual.device_mode);
	config_get("log.mode", &actual.log_mode, sizeof(actual.log_mode));
	LONGS_EQUAL(3, actual.log_mode);
	config_get("chg.mode", &actual.charger.mode, sizeof(actual.charger.mode));
	STRCMP_EQUAL("free", actual.charger.mode);
	config_get("chg.count", &actual.charger.connector_count, sizeof(actual.charger.connector_count));
	LONGS_EQUAL(1, actual.charger.connector_count);
	config_get("chg.c1.plc_mac", &actual.charger.connector[0].plc_mac, sizeof(actual.charger.connector[0].plc_mac));
	MEMCMP_EQUAL("\02\0\0\xfe\xed\01", actual.charger.connector[0].plc_mac, sizeof(actual.charger.connector[0].plc_mac));
	config_get("net.mac", &actual.net.mac, sizeof(actual.net.mac));
	MEMCMP_EQUAL("\0\xf2\0\0\0\0", actual.net.mac, sizeof(actual.net.mac));
	config_get("net.health", &actual.net.health_check_interval, sizeof(actual.net.health_check_interval));
	LONGS_EQUAL(60000, actual.net.health_check_interval);
	config_get("net.server.ping", &actual.net.ping_interval, sizeof(actual.net.ping_interval));
	LONGS_EQUAL(120, actual.net.ping_interval);
	config_get("net.server.url", &actual.net.server_url, sizeof(actual.net.server_url));
	STRCMP_EQUAL("wss://csms.pazzk.net", actual.net.server_url);
}
TEST(Config, ShouldReturnNoEntry_WhenKeyNotFoundReading) {
	struct config actual = { 0, };
	LONGS_EQUAL(-ENOENT, config_get("not.found", &actual, sizeof(actual)));
}
TEST(Config, ShouldReturnPermissionDenied_WhenKeyIsReadonly) {
	struct config actual = { 0, };
	LONGS_EQUAL(-EPERM, config_set("version", &actual.version, sizeof(actual.version)));
}
TEST(Config, ShouldSaveConfigWithCrc_WhenChanged) {
	mock().expectOneCall("write")
		.withStringParameter("key", "basic")
		.withParameter("size", sizeof(config))
		.ignoreOtherParameters()
		.andReturnValue(0);
	mock().expectOneCall("on_config_save")
		.withPointerParameter("ctx", NULL);
	LONGS_EQUAL(0, config_save());
}
TEST(Config, ShouldMatchCrc_WhenSaved) {
	go_default();
	uint32_t crc;
	LONGS_EQUAL(0, config_get("crc", &crc, sizeof(crc)));
	config_read_all(&config);
	LONGS_EQUAL(crc32_cksum((const uint8_t *)&config,
			sizeof(config)-sizeof(config.crc)), crc);
}
TEST(Config, ShouldReturnNoEntry_WhenKeyNotFoundWriting) {
	LONGS_EQUAL(-ENOENT, config_set("not.found", &config, sizeof(config)));
}
TEST(Config, ShouldReturnNoMem_WhenBufferIsTooSmall) {
	uint8_t buf[1];
	LONGS_EQUAL(-ENOMEM, config_get("device.id", buf, sizeof(buf)));
}
TEST(Config, ShouldReturnNoMem_WhenBufferIsTooSmallReadingCustomConfig) {
	uint8_t buf[1];
	LONGS_EQUAL(-ENOMEM, config_get("x509.cert", buf, sizeof(buf)));
}
TEST(Config, ShouldReturnEIO_WhenFlashReadError) {
	uint8_t buf[X509_BUFSIZE];
	mock().expectOneCall("read")
		.ignoreOtherParameters()
		.andReturnValue(-EIO);
	LONGS_EQUAL(-EIO, config_get("x509.cert", buf, sizeof(buf)));
}
TEST(Config, ShouldWriteCustomConfig_WhenX509CertGiven) {
	uint8_t cert[X509_BUFSIZE]= { 1, 2, 3, 4, 5, 6, 7, 8 };
	mock().expectOneCall("write")
		.withStringParameter("key", "x509.cert")
		.withParameter("size", sizeof(cert))
		.ignoreOtherParameters()
		.andReturnValue(0);
	LONGS_EQUAL(0, config_set("x509.cert", cert, sizeof(cert)));
}
TEST(Config, ShouldReadCustomConfig_WhenX509CertRequested) {
	uint8_t cert[X509_BUFSIZE];
	uint8_t expected[X509_BUFSIZE]= { 1, 2, 3, 4, 5, 6, 7, 8 };
	mock().expectOneCall("read")
		.withStringParameter("key", "x509.cert")
		.withOutputParameterReturning("buf", expected, sizeof(expected))
		.withParameter("size", sizeof(expected))
		.andReturnValue(0);
	LONGS_EQUAL(0, config_get("x509.cert", cert, sizeof(cert)));
	MEMCMP_EQUAL(expected, cert, sizeof(cert));
}
TEST(Config, ShouldReturnNoMem_WhenDataSizeIsTooLargeWritingCustomConfig) {
	uint8_t cert[X509_BUFSIZE+1];
	LONGS_EQUAL(-ENOMEM, config_set("x509.cert", cert, sizeof(cert)));
}
TEST(Config, ShouldReturnNoMem_WhenDataSizeIsTooLargeWritingConfig) {
	uint64_t health = 10;
	LONGS_EQUAL(-ENOMEM, config_set("net.health", &health, sizeof(health)));
}
TEST(Config, ShouldReturnEIO_WhenFlashWriteErrorWritingCustomConfig) {
	uint8_t cert[X509_BUFSIZE] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	mock().expectOneCall("write")
		.ignoreOtherParameters()
		.andReturnValue(-EIO);
	LONGS_EQUAL(-EIO, config_set("x509.cert", cert, sizeof(cert)));
}
TEST(Config, ShouldResetAllConfigsToDefault_WhenNullKeyGiven) {
	mock().expectOneCall("clear")
		.withStringParameter("key", "x509.ca")
		.andReturnValue(0);
	mock().expectOneCall("clear")
		.withStringParameter("key", "x509.cert")
		.andReturnValue(0);
	config_reset(NULL);
}
TEST(Config, ShouldResetSpecificConfigToDefault_WhenKeyGiven) {
	uint32_t health = 10;
	LONGS_EQUAL(0, config_set("net.health", &health, sizeof(health)));
	config_reset("net.health");
	config_get("net.health", &health, sizeof(health));
	LONGS_EQUAL(60000, health);
}
TEST(Config, ShouldClearSpecificConfig_WhenCustomKeyGiven) {
	mock().expectOneCall("clear")
		.withStringParameter("key", "x509.cert")
		.andReturnValue(0);
	config_reset("x509.cert");
}
TEST(Config, ShouldReturnNoEntry_WhenGivenKeyNotFound) {
	LONGS_EQUAL(-ENOENT, config_reset("not.found"));
}
