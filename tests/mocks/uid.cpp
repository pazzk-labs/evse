#include "CppUTestExt/MockSupport.h"
#include "uid.h"

struct uid_store *uid_store_create(const struct uid_store_config *config) {
	return (struct uid_store *)mock().actualCall(__func__)
		.returnPointerValue();
}

void uid_store_destroy(struct uid_store *store) {
	mock().actualCall(__func__)
		.withPointerParameter("store", store);
}

int uid_update(struct uid_store *store, const uid_id_t id,
		const uid_id_t pid, uid_status_t status, time_t expiry) {
	return mock().actualCall(__func__)
		.withPointerParameter("store", store)
		.withMemoryBufferParameter("id", id, sizeof(uid_id_t))
		.withMemoryBufferParameter("pid", pid, sizeof(uid_id_t))
		.withParameter("status", status)
		.withParameter("expiry", expiry)
		.returnIntValue();
}

int uid_delete(struct uid_store *store, const uid_id_t id) {
	return mock().actualCall(__func__)
		.withPointerParameter("store", store)
		.withMemoryBufferParameter("id", id, sizeof(uid_id_t))
		.returnIntValue();
}

uid_status_t uid_status(struct uid_store *store,
		const uid_id_t id, uid_id_t pid, time_t *expiry) {
	return (uid_status_t)mock().actualCall(__func__)
		.withPointerParameter("store", store)
		.withMemoryBufferParameter("id", id, sizeof(uid_id_t))
		.withOutputParameter("pid", pid)
		.withOutputParameter("expiry", expiry)
		.returnUnsignedIntValue();
}

int uid_clear(struct uid_store *store) {
	return mock().actualCall(__func__)
		.withPointerParameter("store", store)
		.returnIntValue();
}

int uid_register_update_cb(struct uid_store *store,
		uid_update_cb_t cb, void *ctx) {
	return mock().actualCall(__func__)
		.withPointerParameter("store", store)
		.withPointerParameter("cb", (void *)cb)
		.withPointerParameter("ctx", ctx)
		.returnIntValue();
}

const char *uid_stringify_status(uid_status_t status)
{
	switch (status) {
	case UID_STATUS_ACCEPTED:
		return "Accepted";
	case UID_STATUS_BLOCKED:
		return "Blocked";
	case UID_STATUS_EXPIRED:
		return "Expired";
	case UID_STATUS_INVALID:
		return "Invalid";
	case UID_STATUS_NO_ENTRY:
		return "No Entry";
	default:
		return "Unknown";
	}
}
