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
