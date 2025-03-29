#include "CppUTestExt/MockSupport.h"
#include "safety.h"

struct safety *safety_create(void) {
	return (struct safety *)mock().actualCall("safety_create").returnPointerValue();
}

void safety_destroy(struct safety *self) {
	mock().actualCall("safety_destroy").withPointerParameter("self", self);
}

int safety_add(struct safety *self, struct safety_entry *entry) {
	return mock().actualCall("safety_add")
		.withPointerParameter("self", self)
		.withPointerParameter("entry", entry)
		.returnIntValue();
}

int safety_add_and_enable(struct safety *self, struct safety_entry *entry) {
	return mock().actualCall("safety_add_and_enable")
		.withPointerParameter("self", self)
		.withPointerParameter("entry", entry)
		.returnIntValue();
}

int safety_remove(struct safety *self, struct safety_entry *entry) {
	return mock().actualCall("safety_remove")
		.withPointerParameter("self", self)
		.withPointerParameter("entry", entry)
		.returnIntValue();
}

int safety_check(struct safety *self, safety_error_callback_t cb, void *cb_ctx) {
	return mock().actualCall("safety_check")
		.withPointerParameter("self", self)
		.withPointerParameter("cb", (void *)cb)
		.withPointerParameter("cb_ctx", cb_ctx)
		.returnIntValue();
}

int safety_iterate(struct safety *self, safety_iterator_t cb, void *cb_ctx) {
	return mock().actualCall("safety_iterate")
		.withPointerParameter("self", self)
		.withPointerParameter("cb", (void *)cb)
		.withPointerParameter("cb_ctx", cb_ctx)
		.returnIntValue();
}
