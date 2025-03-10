#include "CppUTestExt/MockSupport.h"
#include "libmcu/kvstore.h"

#include <stdlib.h>

struct kvstore *mock_kvstore_create(void);
void mock_kvstore_destroy(struct kvstore *kvstore);

static int write_mock(struct kvstore *self, const char *key, const void *value, size_t size) {
	return mock().actualCall("write")
		.withStringParameter("key", key)
		.withMemoryBufferParameter("value", (const uint8_t *)value, size)
		.withParameter("size", size)
		.returnIntValue();
}

static int read_mock(struct kvstore *self, const char *key, void *buf, size_t size) {
	return mock().actualCall("read")
		.withStringParameter("key", key)
		.withOutputParameter("buf", buf)
		.withParameter("size", size)
		.returnIntValue();
}

static int clear_mock(struct kvstore *self, const char *key) {
	return mock().actualCall("clear")
		.withStringParameter("key", key)
		.returnIntValue();
}

static int open_mock(struct kvstore *self, const char *ns) {
	return mock().actualCall("open")
		.withPointerParameter("self", self)
		.withStringParameter("ns", ns)
		.returnIntValue();
}

static void close_mock(struct kvstore *self) {
	mock().actualCall("close")
		.withPointerParameter("self", self);
}

struct kvstore *mock_kvstore_create(void) {
	struct kvstore_api *kvstore = new kvstore_api;
	kvstore->write = write_mock;
	kvstore->read = read_mock;
	kvstore->clear = clear_mock;
	kvstore->open = open_mock;
	kvstore->close = close_mock;
	return (struct kvstore *)kvstore;
}

void mock_kvstore_destroy(struct kvstore *kvstore) {
	struct kvstore_api *kvstore_api = (struct kvstore_api *)kvstore;
	delete kvstore_api;
}
