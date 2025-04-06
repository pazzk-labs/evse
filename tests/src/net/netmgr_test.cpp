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

#include <semaphore.h>

#include "net/netmgr.h"
#include "net/ping.h"
#include "net/ntp.h"
#include "libmcu/apptmr.h"
#include "libmcu/metrics.h"
#include "libmcu/board.h"
#include "netif_mock.h"
#include "app.h"

static struct apptmr *ping_timer;
static uint8_t mac[NETIF_MAC_ADDR_LEN] = {0x00, 0xf2, 0x00, 0x00, 0x00, 0x00};

static void on_net_event(const netmgr_state_t event, void *ctx) {
	mock().actualCall(__func__)
		.withParameter("event", event)
		.withParameter("ctx", ctx);
}

static bool mytask(void *ctx) {
	return (bool)mock().actualCall(__func__)
		.withParameter("ctx", ctx).returnBoolValueOrDefault(false);
}

void app_reboot(void) {
	mock().actualCall(__func__);
}

void apptmr_create_hook(struct apptmr *self) {
	ping_timer = self;
}

int ping_measure(const char *ipstr, const int timeout_ms)
{
	return mock().actualCall(__func__)
		.withParameter("ipstr", ipstr)
		.withParameter("timeout_ms", timeout_ms)
		.returnIntValue();
}

int ntp_start(void (*cb)(struct timeval *, void *ctx), void *cb_ctx) {
	return mock().actualCall(__func__)
		.withParameter("cb", cb)
		.withParameter("cb_ctx", cb_ctx)
		.returnIntValue();
}

uint32_t board_get_time_since_boot_ms(void) {
	return (uint32_t)mock().actualCall(__func__)
		.returnUnsignedIntValue();
}

uint32_t board_get_current_stack_watermark(void) {
	return 0;
}

uint32_t board_random(void) {
	return 0;
}

TEST_GROUP(NetworkManager) {
	struct netif *netif;
	uint32_t time;

	void setup(void) {
		SimpleString groupName = UtestShell::getCurrent()->getGroup();
		SimpleString testName = UtestShell::getCurrent()->getName();
		printf("Running test: %s::%s\n",
				groupName.asCharString(),
				testName.asCharString());

		set_time(0);
		metrics_init(true);
		netmgr_init(60000);
		netmgr_register_event_cb(NETMGR_STATE_ANY, on_net_event, NULL);
	}
	void teardown(void) {
		netmgr_deinit();

		mock().checkExpectations();
		mock().clear();
	}

	void set_time(uint32_t t) {
		time = t;
	}
	uint32_t get_time(void) {
		return time;
	}

	void register_netif(void) {
		netif = mock_netif_create();
		netmgr_register_iface(netif, 0, mac, NULL);
	}

	void step(int n) {
		for (int i = 0; i < n; i++) {
			mock().expectOneCall("board_get_time_since_boot_ms")
				.andReturnValue(get_time());
			netmgr_step();
		}
	}

	void go_enabled(void) {
		register_netif();
		netmgr_enable();
		step(2);
		set_time(1000); // re-enable delay
		step(2);
	}
	void go_connected(void) {
		go_enabled();
		step(1);
		mock_netif_raise_event(netif, NETIF_EVENT_IP_ACQUIRED);
		step(1);
	}
};

TEST(NetworkManager, state_ShouldReturnOFF_WhenOff) {
	LONGS_EQUAL(NETMGR_STATE_OFF, netmgr_state());
}
TEST(NetworkManager, state_ShouldReturnInitialized_WhenTurnedOnAfterNetIFRegistered) {
	mock().ignoreOtherCalls();
	register_netif();
	netmgr_enable();
	step(3);
	LONGS_EQUAL(NETMGR_STATE_INITIALIZED, netmgr_state());
}
TEST(NetworkManager, state_ShouldReturnEnabled_WhenEnabled) {
	mock().ignoreOtherCalls();
	go_enabled();
	LONGS_EQUAL(NETMGR_STATE_ENABLED, netmgr_state());
}
TEST(NetworkManager, state_ShouldReturnConnected_WhenConnected) {
	mock().ignoreOtherCalls();
	go_connected();
	LONGS_EQUAL(NETMGR_STATE_CONNECTED, netmgr_state());
}
TEST(NetworkManager, state_ShouldReturnDisconnected_WhenDisconnected) {
	mock().ignoreOtherCalls();
	go_connected();
	LONGS_EQUAL(NETMGR_STATE_CONNECTED, netmgr_state());
	mock_netif_raise_event(netif, NETIF_EVENT_DISCONNECTED);
	step(1);
	LONGS_EQUAL(NETMGR_STATE_DISCONNECTED, netmgr_state());
}
TEST(NetworkManager, state_ShouldReturnExhausted_WhenRetryExceeded) {
	mock().ignoreOtherCalls();
	go_enabled();
	uint32_t t = get_time();
	for (unsigned i = 0; i < NETMGR_MAX_RETRY; i++) {
		set_time(get_time() + NETMGR_CONNECT_TIMEOUT_MS);
		step(1);
		t *= 2;
		set_time(t);
		step(1);
	}
	step(1);
	LONGS_EQUAL(NETMGR_STATE_EXHAUSTED, netmgr_state());
}
TEST(NetworkManager, ShouldGoOff_WhenDisabled) {
	mock().ignoreOtherCalls();
	go_enabled();
	netmgr_disable();
	step(1);
	LONGS_EQUAL(NETMGR_STATE_OFF, netmgr_state());
}
TEST(NetworkManager, ShouldCallEventCallback_WhenEventHappens) {
	mock().ignoreOtherCalls();
	mock().expectOneCall("on_net_event")
		.withParameter("event", NETMGR_STATE_INITIALIZED)
		.ignoreOtherParameters();
	mock().expectOneCall("on_net_event")
		.withParameter("event", NETMGR_STATE_ENABLED)
		.ignoreOtherParameters();
	mock().expectOneCall("on_net_event")
		.withParameter("event", NETMGR_STATE_CONNECTED)
		.ignoreOtherParameters();
	go_connected();

	mock().expectOneCall("on_net_event")
		.withParameter("event", NETMGR_STATE_DISCONNECTED)
		.ignoreOtherParameters();
	mock().expectOneCall("on_net_event")
		.withParameter("event", NETMGR_STATE_DISABLED)
		.ignoreOtherParameters();
	mock().expectOneCall("on_net_event")
		.withParameter("event", NETMGR_STATE_OFF)
		.ignoreOtherParameters();
	netmgr_disable();
	step(1);
	LONGS_EQUAL(NETMGR_STATE_OFF, netmgr_state());
}
TEST(NetworkManager, connected_ShouldReturnTrue_WhenConnected) {
	mock().ignoreOtherCalls();
	go_connected();
	LONGS_EQUAL(true, netmgr_connected());
}
TEST(NetworkManager, connected_ShouldReturnFalse_WhenNotConnected) {
	mock().ignoreOtherCalls();
	go_enabled();
	LONGS_EQUAL(false, netmgr_connected());
}
TEST(NetworkManager, ShouldPingPeriodically_WhenConnected) {
	mock().ignoreOtherCalls();
	go_connected();
	apptmr_trigger(ping_timer);
	mock().expectOneCall("ping_measure")
		.withParameter("ipstr", "0.0.0.0")
		.withParameter("timeout_ms", 100)
		.andReturnValue(0);
	step(1);
}
TEST(NetworkManager, ShouldRetryConnecting_WhenPingFailed) {
	mock().ignoreOtherCalls();
	go_connected();
	apptmr_trigger(ping_timer);
	mock().expectOneCall("ping_measure")
		.withParameter("ipstr", "0.0.0.0")
		.withParameter("timeout_ms", 100)
		.andReturnValue(-1);
	step(2);
	LONGS_EQUAL(NETMGR_STATE_DISABLED, netmgr_state());
	set_time(get_time() + 1000); // re-enable delay
	step(1);
	LONGS_EQUAL(NETMGR_STATE_INITIALIZED, netmgr_state());
}
TEST(NetworkManager, ShouldStartNTP_WhenConnected) {
	mock().ignoreOtherCalls();
	mock().expectOneCall("ntp_start").ignoreOtherParameters().andReturnValue(0);
	go_connected();
	step(1);
}
TEST(NetworkManager, ShouldCallRegisteredUserTask_WhenConnected) {
	mock().ignoreOtherCalls();
	netmgr_register_task(mytask, NULL);
	mock().expectOneCall("mytask").ignoreOtherParameters();
	go_connected();
	step(1);
}

TEST(NetworkManager, ShouldInitializeNetif_WhenEnabled) {
	mock().expectOneCall("init").ignoreOtherParameters();
	mock().expectOneCall("register_event_callback").ignoreOtherParameters();
	mock().expectOneCall("on_net_event")
		.withParameter("event", NETMGR_STATE_INITIALIZED)
		.ignoreOtherParameters();

	register_netif();
	netmgr_enable();
	step(2);
}

TEST(NetworkManager, ShouldReconnectWithBackoff_WhenConnectFailed) {
	mock().ignoreOtherCalls();
	go_enabled();

	mock().expectOneCall("connect").ignoreOtherParameters().andReturnValue(-1);
	step(2);
	LONGS_EQUAL(NETMGR_STATE_DISABLED, netmgr_state());
	set_time(get_time() + 1000); // re-enable delay
	step(3);
	set_time(get_time() + 1000); // backoff
	mock().expectOneCall("connect").ignoreOtherParameters().andReturnValue(0);
	step(1);
}
TEST(NetworkManager, ShouldGoOff_WhenConnectFailedTooMany) {
	mock().ignoreOtherCalls();
	go_enabled();
	uint32_t t = get_time();

	for (unsigned i = 0; i < NETMGR_MAX_RETRY; i++) {
		t *= 2;
		set_time(t);
		mock().expectOneCall("connect").ignoreOtherParameters().andReturnValue(-1);
		step(2);
		set_time(t + 1000); // re-enable delay
		step(3);
	}
	LONGS_EQUAL(NETMGR_STATE_EXHAUSTED, netmgr_state());
	step(3);
	LONGS_EQUAL(NETMGR_STATE_EXHAUSTED, netmgr_state());
}
TEST(NetworkManager, ShouldRetryConnecting_WhenReenabledAfterConnectFailedTooMany) {
	mock().ignoreOtherCalls();
	go_enabled();
	uint32_t t = get_time();

	for (unsigned i = 0; i < NETMGR_MAX_RETRY; i++) {
		t *= 2;
		set_time(t);
		mock().expectOneCall("connect").ignoreOtherParameters().andReturnValue(-1);
		step(2);
		set_time(t + 1000); // re-enable delay
		step(3);
	}
	LONGS_EQUAL(NETMGR_STATE_EXHAUSTED, netmgr_state());
	netmgr_enable();
	step(2);
	set_time(get_time() + 1000); // re-enable delay
	mock().expectOneCall("enable").ignoreOtherParameters().andReturnValue(0);
	mock().expectOneCall("connect").ignoreOtherParameters().andReturnValue(0);
	step(3);
}

TEST(NetworkManager, ShouldReinitialze_WhenEnableFailed) {
	mock().ignoreOtherCalls();
	register_netif();
	netmgr_enable();
	step(2);
	mock().expectOneCall("enable").ignoreOtherParameters().andReturnValue(-1);
	set_time(1000); // re-enable delay
	step(2);
	LONGS_EQUAL(NETMGR_STATE_OFF, netmgr_state());
}
TEST(NetworkManager, ShouldGoOff_WhenEnableFailedTooMany) {
	mock().ignoreOtherCalls();
	register_netif();
	netmgr_enable();
	step(2);

	for (unsigned i = 0; i < NETMGR_MAX_RETRY; i++) {
		set_time(get_time() + 1000);
		mock().expectOneCall("enable").ignoreOtherParameters().andReturnValue(-1);
		step(4);
	}
	LONGS_EQUAL(NETMGR_STATE_OFF, netmgr_state());
	step(2);
	LONGS_EQUAL(NETMGR_STATE_OFF, netmgr_state());
}
TEST(NetworkManager, ShouldRetryConnecting_WhenReenabledAfterTooManyErrors) {
	mock().ignoreOtherCalls();
	register_netif();
	netmgr_enable();
	step(2);

	for (unsigned i = 0; i < NETMGR_MAX_RETRY; i++) {
		set_time(get_time() + 1000);
		mock().expectOneCall("enable").ignoreOtherParameters().andReturnValue(-1);
		step(4);
	}
	LONGS_EQUAL(NETMGR_STATE_OFF, netmgr_state());
	netmgr_enable();
	step(2);
	set_time(get_time() + 1000); // re-enable delay
	mock().expectOneCall("enable").ignoreOtherParameters().andReturnValue(0);
	step(2);
	LONGS_EQUAL(NETMGR_STATE_ENABLED, netmgr_state());
}
TEST(NetworkManager, ShouldDoNothing_WhenNoInterfaceRegistered) {
	netmgr_enable();
	netmgr_step();
	LONGS_EQUAL(NETMGR_STATE_OFF, netmgr_state());
}

TEST(NetworkManager, ShouldReenable_WhenEnablingTakesTooLong) {
	mock().ignoreOtherCalls();
	register_netif();
	netmgr_enable();
	step(2);
	set_time(1000); // re-enable delay
	step(1);
	set_time(get_time() + NETMGR_CONNECT_TIMEOUT_MS);
	step(1);
	LONGS_EQUAL(NETMGR_STATE_DISABLED, netmgr_state());
	set_time(get_time() + 1000);
	step(1);
	LONGS_EQUAL(NETMGR_STATE_INITIALIZED, netmgr_state());
}

TEST(NetworkManager, ShouldRetryConnecting_WhenDisconnected) {
	mock().ignoreOtherCalls();
	go_connected();
	mock_netif_raise_event(netif, NETIF_EVENT_DISCONNECTED);
	step(1);
	LONGS_EQUAL(NETMGR_STATE_DISCONNECTED, netmgr_state());
	set_time(get_time() + 1000); // backoff
	mock().expectOneCall("connect").ignoreOtherParameters().andReturnValue(0);
	step(1);
}

TEST(NetworkManager, PingShouldReturnENODEV_WhenNoInterfaceGiven) {
	LONGS_EQUAL(-ENODEV, netmgr_ping("1.1.1.1", 100));
}
TEST(NetworkManager, PingShouldReturnENOTCONN_WhenNotConnected) {
	mock().ignoreOtherCalls();
	go_enabled();
	LONGS_EQUAL(-ENOTCONN, netmgr_ping("1.1.1.1", 100));
}
TEST(NetworkManager, ShouldUsedHighestPriority_WhenMultipleInterfacesRegistered) {
}
