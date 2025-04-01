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

#include "net/util.h"

TEST_GROUP(NetUtil) {
	void setup(void) {
	}
	void teardown(void) {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(NetUtil, ShouldReturnTrueWhenProtocolIsSecure) {
	CHECK_TRUE(net_is_secure_protocol(NET_PROTO_HTTPS));
	CHECK_TRUE(net_is_secure_protocol(NET_PROTO_WSS));
	CHECK_TRUE(net_is_secure_protocol(NET_PROTO_MQTTS));
	CHECK_TRUE(net_is_secure_protocol(NET_PROTO_FTPS));
	CHECK_TRUE(net_is_secure_protocol(NET_PROTO_SFTP));
}
TEST(NetUtil, ShouldReturnFalseWhenProtocolIsNotSecure) {
	CHECK_FALSE(net_is_secure_protocol(NET_PROTO_HTTP));
	CHECK_FALSE(net_is_secure_protocol(NET_PROTO_WS));
	CHECK_FALSE(net_is_secure_protocol(NET_PROTO_MQTT));
	CHECK_FALSE(net_is_secure_protocol(NET_PROTO_FTP));
}
TEST(NetUtil, ShouldReturnUnknownProtocol_WhenUnknownProtocolGiven) {
	CHECK_EQUAL(NET_PROTO_UNKNOWN, net_get_protocol_from_url("unknown://"));
}
TEST(NetUtil, ShouldReturnHttpProtocol_WhenHttpProtocolGiven) {
	CHECK_EQUAL(NET_PROTO_HTTP, net_get_protocol_from_url("http://"));
}
TEST(NetUtil, ShouldReturnHttpsProtocol_WhenHttpsProtocolGiven) {
	CHECK_EQUAL(NET_PROTO_HTTPS, net_get_protocol_from_url("https://"));
}
TEST(NetUtil, ShouldReturnUnknownProtocol_WhenEmptyStringGiven) {
	CHECK_EQUAL(NET_PROTO_UNKNOWN, net_get_protocol_from_url(""));
}
TEST(NetUtil, ShouldReturnUnknownProtocol_WhenInvalidProtocolGiven) {
	CHECK_EQUAL(NET_PROTO_UNKNOWN, net_get_protocol_from_url("http//"));
	CHECK_EQUAL(NET_PROTO_UNKNOWN, net_get_protocol_from_url("http:/"));
	CHECK_EQUAL(NET_PROTO_UNKNOWN, net_get_protocol_from_url("http:/"));
	CHECK_EQUAL(NET_PROTO_UNKNOWN, net_get_protocol_from_url("http;//"));
}

TEST(NetUtil, ShouldReturnTrueWhenIpv4IsValid) {
	CHECK_TRUE(net_is_ipv4_str_valid("192.168.0.1"));
}
TEST(NetUtil, ShouldInterpretDotWithoutNumberIsZero) {
	CHECK_TRUE(net_is_ipv4_str_valid("1.2.3."));
	CHECK_TRUE(net_is_ipv4_str_valid(".1.2.3"));
	CHECK_TRUE(net_is_ipv4_str_valid("1..2.3"));
}
TEST(NetUtil, ShouldReturnFalseWhenIpv4IsInvalid) {
	CHECK_FALSE(net_is_ipv4_str_valid("192.168.0.256"));
	CHECK_FALSE(net_is_ipv4_str_valid("192.168.0"));
}
TEST(NetUtil, ShouldReturnFalseWhenIpv4IsInvalid_WhenEmptyStringGiven) {
	CHECK_FALSE(net_is_ipv4_str_valid(""));
}

TEST(NetUtil, ShouldReturnMac_WhenValidStringGiven) {
	const uint8_t expected[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };
	uint8_t actual[6] = {0, };
	LONGS_EQUAL(true, net_get_mac_from_str("00:11:22:33:44:55", actual));
	MEMCMP_EQUAL(expected, actual, sizeof(expected));
}
TEST(NetUtil, ShouldReturnMac_WhenValidStringGiven_WithoutColon) {
	const uint8_t expected[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };
	uint8_t actual[6] = {0, };
	LONGS_EQUAL(true, net_get_mac_from_str("001122334455", actual));
	MEMCMP_EQUAL(expected, actual, sizeof(expected));
}
TEST(NetUtil, ShouldReturnFalse_WhenInvalidStringGiven) {
	uint8_t actual[6] = {0, };
	LONGS_EQUAL(false, net_get_mac_from_str("00:11:22:33:44:55:", actual));
	LONGS_EQUAL(false, net_get_mac_from_str("00:11:22:33:44:55:66", actual));
	LONGS_EQUAL(false, net_get_mac_from_str("00:11:22:33:44:55:66:77", actual));
	LONGS_EQUAL(false, net_get_mac_from_str("00:11:22:33:44:55:6", actual));
	LONGS_EQUAL(false, net_get_mac_from_str("00:11:22:33:44:5", actual));
	LONGS_EQUAL(false, net_get_mac_from_str("00:11:22:33:44:", actual));
	LONGS_EQUAL(false, net_get_mac_from_str("00:11:22:33:44", actual));
	LONGS_EQUAL(false, net_get_mac_from_str("00:11:22:33:4", actual));
	LONGS_EQUAL(false, net_get_mac_from_str("00:11:22:33:", actual));
	LONGS_EQUAL(false, net_get_mac_from_str("00:11:22:33", actual));
	LONGS_EQUAL(false, net_get_mac_from_str("00:11:22:3", actual));
	LONGS_EQUAL(false, net_get_mac_from_str("00:11:22:", actual));
	LONGS_EQUAL(false, net_get_mac_from_str("00:11:22", actual));
	LONGS_EQUAL(false, net_get_mac_from_str("00:11:2", actual));
	LONGS_EQUAL(false, net_get_mac_from_str("00:11:", actual));
	LONGS_EQUAL(false, net_get_mac_from_str("00:11", actual));
	LONGS_EQUAL(false, net_get_mac_from_str("00:1", actual));
	LONGS_EQUAL(false, net_get_mac_from_str("00:", actual));
	LONGS_EQUAL(false, net_get_mac_from_str("00", actual));
	LONGS_EQUAL(false, net_get_mac_from_str("0", actual));
	LONGS_EQUAL(false, net_get_mac_from_str("", actual));
}
TEST(NetUtil, ShouldReturnFalse_WhenEmptyStringGiven) {
	uint8_t actual[6] = {0, };
	LONGS_EQUAL(false, net_get_mac_from_str("", actual));
}
TEST(NetUtil, ShouldReturnFalse_WhenNotHexStringGiven) {
	uint8_t actual[6] = {0, };
	LONGS_EQUAL(false, net_get_mac_from_str("00:11:22:33:44:5G", actual));
}
TEST(NetUtil, ShouldReturnTrue_WhenUpperOrLowerCaseHexStringGiven) {
	uint8_t actual[6] = {0, };
	LONGS_EQUAL(true, net_get_mac_from_str("aa:bb:cc:dd:ee:ff", actual));
	MEMCMP_EQUAL("\xaa\xbb\xcc\xdd\xee\xff", actual, 6);
	LONGS_EQUAL(true, net_get_mac_from_str("AA:BB:CC:DD:EE:FF", actual));
	MEMCMP_EQUAL("\xaa\xbb\xcc\xdd\xee\xff", actual, 6);
}

TEST(NetUtil, ShouldReturnMacString_WhenValidMacGiven) {
	const uint8_t mac[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };
	char buf[18] = {0, };
	net_stringify_mac(mac, buf, sizeof(buf));
	STRCMP_EQUAL("00:11:22:33:44:55", buf);
}

TEST(NetUtil, GetHostFromUrl_ValidUrl) {
	const char *url = "http://example.com/path";
	char buffer[256] = {0, };
	int result = net_get_host_from_url(url, buffer, sizeof(buffer));
	STRCMP_EQUAL("example.com", buffer);
	CHECK_EQUAL(0, result);
}
TEST(NetUtil, GetHostFromUrl_ValidUrl_WhenPortGiven) {
	const char *url = "http://example.com:8080/path";
	char buffer[256] = {0, };
	int result = net_get_host_from_url(url, buffer, sizeof(buffer));
	STRCMP_EQUAL("example.com", buffer);
	CHECK_EQUAL(0, result);
}
TEST(NetUtil, GetHostFromUrl_InvalidUrl) {
	const char *url = "invalid_url";
	char buffer[256] = {0, };
	int result = net_get_host_from_url(url, buffer, sizeof(buffer));
	CHECK_EQUAL(-EINVAL, result);
}
TEST(NetUtil, GetHostFromUrl_BufferTooSmall) {
	const char *url = "http://example.com/path";
	char buffer[256] = {0, };
	int result = net_get_host_from_url(url, buffer, 5); // Buffer too small
	STRCMP_EQUAL("exam", buffer); // Partial copy
	CHECK_EQUAL(0, result);
}

TEST(NetUtil, GetPathFromUrl_ValidUrl) {
	const char *url = "http://example.com/path/to/resource";
	char buffer[256] = {0, };
	int result = net_get_path_from_url(url, buffer, sizeof(buffer));
	STRCMP_EQUAL("/path/to/resource", buffer);
	CHECK_EQUAL(0, result);
}
TEST(NetUtil, GetPathFromUrl_ValidUrl_WhenPortGiven) {
	const char *url = "http://example.com:8080/path/to/resource";
	char buffer[256] = {0, };
	int result = net_get_path_from_url(url, buffer, sizeof(buffer));
	STRCMP_EQUAL("/path/to/resource", buffer);
	CHECK_EQUAL(0, result);
}
TEST(NetUtil, GetPathFromUrl_NoPath) {
	const char *url = "http://example.com";
	char buffer[256] = {0, };
	int result = net_get_path_from_url(url, buffer, sizeof(buffer));
	STRCMP_EQUAL("/", buffer); // Slash should be returned
	CHECK_EQUAL(0, result);
}
TEST(NetUtil, GetPathFromUrl_InvalidUrl) {
	const char *url = "invalid_url";
	char buffer[256] = {0, };
	int result = net_get_path_from_url(url, buffer, sizeof(buffer));
	CHECK_EQUAL(-EINVAL, result);
}
TEST(NetUtil, GetPathFromUrl_BufferTooSmall) {
	const char *url = "http://example.com/path/to/resource";
	char buffer[256] = {0, };
	int result = net_get_path_from_url(url, buffer, 10); // Buffer too small
	STRCMP_EQUAL("/path/to/", buffer); // Partial copy
	CHECK_EQUAL(0, result);
}
TEST(NetUtil, GetPathFromUrl_NoPathWithSlash) {
	const char *url = "http://example.com/";
	char buffer[256] = {0, };
	int result = net_get_path_from_url(url, buffer, sizeof(buffer));
	STRCMP_EQUAL("/", buffer); // Slash should be returned
	CHECK_EQUAL(0, result);
}
TEST(NetUtil, GetPathFromUrl_NoPathWithoutSlash) {
	const char *url = "http://example.com";
	char buffer[256] = {0, };
	int result = net_get_path_from_url(url, buffer, sizeof(buffer));
	STRCMP_EQUAL("/", buffer); // Slash should be returned even if no slash in URL
	CHECK_EQUAL(0, result);
}
TEST(NetUtil, GetPathFromUrl_NoPathWithSlash_WhenPortGiven) {
	const char *url = "http://example.com:8080/";
	char buffer[256] = {0, };
	int result = net_get_path_from_url(url, buffer, sizeof(buffer));
	STRCMP_EQUAL("/", buffer); // Slash should be returned
	CHECK_EQUAL(0, result);
}
TEST(NetUtil, GetPathFromUrl_NoPathWithoutSlash_WhenPortGiven) {
	const char *url = "http://example.com:8080";
	char buffer[256] = {0, };
	int result = net_get_path_from_url(url, buffer, sizeof(buffer));
	STRCMP_EQUAL("/", buffer); // Slash should be returned even if no slash in URL
	CHECK_EQUAL(0, result);
}

TEST(NetUtil, GetPortFromUrl_ValidUrlWithPort) {
	const char *url = "http://example.com:8080";
	uint16_t port = net_get_port_from_url(url);
	CHECK_EQUAL(8080, port);
}
TEST(NetUtil, GetPortFromUrl_ValidUrlWithPort_WhenPathGiven) {
	const char *url = "http://example.com:8080/path";
	uint16_t port = net_get_port_from_url(url);
	CHECK_EQUAL(8080, port);
}
TEST(NetUtil, GetPortFromUrl_ValidUrlWithoutPort) {
	const char *url = "http://example.com";
	uint16_t port = net_get_port_from_url(url);
	CHECK_EQUAL(0, port); // No port specified, should return 0
}
TEST(NetUtil, GetPortFromUrl_InvalidUrl) {
	const char *url = "invalid_url";
	uint16_t port = net_get_port_from_url(url);
	CHECK_EQUAL(0, port); // Invalid URL, should return 0
}
TEST(NetUtil, GetPortFromUrl_UrlWithInvalidPort) {
	const char *url = "http://example.com:invalid";
	uint16_t port = net_get_port_from_url(url);
	CHECK_EQUAL(0, port); // Invalid port, should return 0
}
