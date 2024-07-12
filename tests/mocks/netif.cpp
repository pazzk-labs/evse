#include "CppUTestExt/MockSupport.h"
#include "netif_mock.h"

struct netif {
	struct netif_api api;
	netif_event_callback_t cb;
	void *cb_ctx;
	bool enabled;
};

static int init(struct netif *netif, uint8_t mac_addr[NETIF_MAC_ADDR_LEN]) {
	return mock().actualCall(__func__)
		.withParameter("netif", netif)
		.withParameter("mac_addr", mac_addr)
		.returnIntValue();
}
static int deinit(struct netif *netif) {
	return mock().actualCall(__func__)
		.withParameter("netif", netif)
		.returnIntValue();
}
static int enable(struct netif *netif) {
	netif->enabled = true;
	return mock().actualCall(__func__)
		.withParameter("netif", netif)
		.returnIntValue();
}
static int disable(struct netif *netif) {
	netif->enabled = false;
	return mock().actualCall(__func__)
		.withParameter("netif", netif)
		.returnIntValue();
}
static int connect(struct netif *netif) {
	return mock().actualCall(__func__)
		.returnIntValue();
}
static int disconnect(struct netif *netif) {
	return mock().actualCall(__func__)
		.returnIntValue();
}
static int register_event_callback(struct netif *netif,
	      const netif_event_t event_type,
	      const netif_event_callback_t cb, void *cb_ctx) {
	netif->cb = cb;
	netif->cb_ctx = cb_ctx;
	return mock().actualCall(__func__)
		.withParameter("netif", netif)
		.withParameter("event_type", event_type)
		.withParameter("cb", cb)
		.withParameter("cb_ctx", cb_ctx)
		.returnIntValue();
}
static int unregister_event_callback(struct netif *netif,
	      const netif_event_t event_type,
	      const netif_event_callback_t cb) {
	return mock().actualCall(__func__)
		.withParameter("netif", netif)
		.withParameter("event_type", event_type)
		.withParameter("cb", cb)
		.returnIntValue();
}
static int set_ip_info(struct netif *netif, const ip_info_t *ip_info) {
	return mock().actualCall(__func__)
		.withParameter("netif", netif)
		.withParameter("ip_info", ip_info)
		.returnIntValue();
}
static int get_ip_info(struct netif *netif, ip_info_t *ip_info) {
	return mock().actualCall(__func__)
		.withParameter("netif", netif)
		.withOutputParameter("ip_info", ip_info)
		.returnIntValue();
}
static int get_speed(struct netif *netif, uint32_t *kbps) {
	return mock().actualCall(__func__)
		.withParameter("netif", netif)
		.withOutputParameter("kbps", kbps)
		.returnIntValue();
}
static int get_duplex(struct netif *netif, bool *duplex_enabled) {
	return mock().actualCall(__func__)
		.withParameter("netif", netif)
		.withOutputParameter("duplex_enabled", duplex_enabled)
		.returnIntValue();
}

void mock_netif_raise_event(struct netif *netif, const netif_event_t event) {
	if (netif->cb) {
		netif->cb(netif, event, netif->cb_ctx);
	}
}

bool mock_netif_enabled(struct netif *netif) {
	return netif->enabled;
}

struct netif *mock_netif_create(void) {
	static struct netif netif = {
		.api = {
			.init = init,
			.deinit = deinit,
			.enable = enable,
			.disable = disable,
			.connect = connect,
			.disconnect = disconnect,
			.register_event_callback = register_event_callback,
			.unregister_event_callback = unregister_event_callback,
			.set_ip_info = set_ip_info,
			.get_ip_info = get_ip_info,
			.get_speed = get_speed,
			.get_duplex = get_duplex,
		},
	};
	return &netif;
}
