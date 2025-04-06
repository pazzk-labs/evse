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

extern "C" {
#include "safety.h"
#include <stdlib.h>
}

static safety_entry_status_t mock_check(struct safety_entry* self) {
	return (safety_entry_status_t)mock().actualCall("check")
		.returnIntValueOrDefault(SAFETY_STATUS_OK);
}
static int mock_enable(struct safety_entry* self) {
	return mock().actualCall("enable").returnIntValueOrDefault(0);
}
static int mock_disable(struct safety_entry* self) {
	return mock().actualCall("disable").returnIntValueOrDefault(0);
}
static int mock_get_frequency(const struct safety_entry* self) {
	return mock().actualCall("get_frequency")
		.returnIntValueOrDefault(60);
}
static void mock_destroy(struct safety_entry* self) {
	free(self);
	mock().actualCall("destroy");
}

static struct safety_entry* allocate_mock_entry(void)
{
	struct safety_entry_api *p =
		(struct safety_entry_api *)malloc(sizeof(*p));

	*p = (struct safety_entry_api) {
		.enable = mock_enable,
		.disable = mock_disable,
		.check = mock_check,
		.get_frequency = mock_get_frequency,
		.destroy = mock_destroy,
	};

	return (struct safety_entry *)p;
}

static void free_mock_entry(struct safety_entry* entry)
{
	free(entry);
}

TEST_GROUP(SafetyTestGroup)
{
	struct safety* ctx;

	void setup() override {
		ctx = safety_create();
		CHECK_TEXT(ctx != nullptr, "Failed to create safety context");
	}

	void teardown() override {
		safety_destroy(ctx);

		mock().checkExpectations();
		mock().clear();
	}
};

TEST(SafetyTestGroup, CanCreateAndDestroySafety)
{
	struct safety_entry *entry = allocate_mock_entry();

	LONGS_EQUAL(0, safety_add(ctx, entry));

	mock().expectOneCall("disable");
	mock().expectOneCall("destroy");
}

TEST(SafetyTestGroup, CanAddAndRemoveEntry)
{
	struct safety_entry* entry = allocate_mock_entry();

	LONGS_EQUAL(0, safety_add(ctx, entry));
	LONGS_EQUAL(0, safety_remove(ctx, entry));

	free_mock_entry(entry); // 이 경우 destroy는 호출되지 않음
}

TEST(SafetyTestGroup, CheckReturnsZeroIfAllEntriesOK)
{
	struct safety_entry* entry = allocate_mock_entry();

	mock().expectOneCall("check").andReturnValue(SAFETY_STATUS_OK);

	LONGS_EQUAL(0, safety_add(ctx, entry));

	int rc = safety_check(ctx, nullptr, nullptr);
	LONGS_EQUAL(0, rc);

	mock().expectOneCall("disable");
	mock().expectOneCall("destroy");
}

static void error_callback(struct safety_entry* entry, safety_entry_status_t status, void* ctx)
{
	int* counter = static_cast<int*>(ctx);
	(*counter)++;
}

TEST(SafetyTestGroup, CheckReportsErrorViaCallback)
{
	struct safety_entry* entry = allocate_mock_entry();

	mock().expectOneCall("check").andReturnValue(SAFETY_STATUS_EMERGENCY_STOP);

	safety_add(ctx, entry);

	int error_count = 0;
	int rc = safety_check(ctx, error_callback, &error_count);
	LONGS_EQUAL(1, rc);
	LONGS_EQUAL(1, error_count);

	mock().expectOneCall("disable");
	mock().expectOneCall("destroy");
}

static void iter_callback(struct safety_entry* entry, void* ctx)
{
	int* count = static_cast<int*>(ctx);
	(*count)++;
}

TEST(SafetyTestGroup, IterateInvokesCallbackForEachEntry)
{
	struct safety_entry* entry1 = allocate_mock_entry();
	struct safety_entry* entry2 = allocate_mock_entry();

	safety_add(ctx, entry1);
	safety_add(ctx, entry2);

	int count = 0;
	safety_iterate(ctx, iter_callback, &count);
	LONGS_EQUAL(2, count);

	mock().expectNCalls(2, "disable");
	mock().expectNCalls(2, "destroy");
}

TEST(SafetyTestGroup, AddNullEntryReturnsEINVAL) {
	LONGS_EQUAL(-EINVAL, safety_add(ctx, nullptr));
}

TEST(SafetyTestGroup, RemoveNullEntryReturnsEINVAL) {
	LONGS_EQUAL(-EINVAL, safety_remove(ctx, nullptr));
}

TEST(SafetyTestGroup, RemoveUnregisteredEntryReturnsENOENT) {
	struct safety_entry* entry = allocate_mock_entry();
	LONGS_EQUAL(-ENOENT, safety_remove(ctx, entry));
	free_mock_entry(entry);
}

TEST(SafetyTestGroup, AddSameEntryTwiceShouldFailIfImplemented) {
	struct safety_entry* entry = allocate_mock_entry();
	LONGS_EQUAL(0, safety_add(ctx, entry));
	LONGS_EQUAL(-EALREADY, safety_add(ctx, entry));
	LONGS_EQUAL(0, safety_remove(ctx, entry));
	free_mock_entry(entry);
}

TEST(SafetyTestGroup, SafetyCheckWithNullSelfReturnsEINVAL) {
	LONGS_EQUAL(-EINVAL, safety_check(nullptr, nullptr, nullptr));
}

TEST(SafetyTestGroup, SafetyIterateWithNullSelfReturnsEINVAL) {
	LONGS_EQUAL(-EINVAL, safety_iterate(nullptr, nullptr, nullptr));
}

TEST(SafetyTestGroup, SafetyIterateWithNullCallbackShouldReturnEINVAL) {
	struct safety_entry* entry = allocate_mock_entry();
	safety_add(ctx, entry);
	LONGS_EQUAL(-EINVAL, safety_iterate(ctx, nullptr, nullptr));
	mock().expectOneCall("disable");
	mock().expectOneCall("destroy");
}

TEST(SafetyTestGroup, CheckFailsButCallbackIsNull) {
	struct safety_entry* entry = allocate_mock_entry();
	safety_add(ctx, entry);
	mock().expectOneCall("check").andReturnValue(SAFETY_STATUS_STALE);
	LONGS_EQUAL(1, safety_check(ctx, nullptr, nullptr));
	mock().expectOneCall("disable");
	mock().expectOneCall("destroy");
}

TEST(SafetyTestGroup, DestroyHandlesNullPointerGracefully) {
	safety_destroy(nullptr); // 기대: crash 없어야 함
}

TEST(SafetyTestGroup, DisableFailsButDestroyStillCalled) {
	struct safety_entry* entry = allocate_mock_entry();
	safety_add(ctx, entry);
	mock().expectOneCall("disable").andReturnValue(-1);
	mock().expectOneCall("destroy");
}

TEST(SafetyTestGroup, MultipleEntriesWithPartialFailure) {
	struct safety_entry* entry1 = allocate_mock_entry();
	struct safety_entry* entry2 = allocate_mock_entry();
	safety_add(ctx, entry1);
	safety_add(ctx, entry2);

	mock().expectOneCall("check").andReturnValue(SAFETY_STATUS_OK);
	mock().expectOneCall("check").andReturnValue(SAFETY_STATUS_EMERGENCY_STOP);

	int error_count = 0;
	LONGS_EQUAL(1, safety_check(ctx, [](struct safety_entry*, safety_entry_status_t, void* t) {
		int* c = static_cast<int*>(t);
		(*c)++;
	}, &error_count));
	LONGS_EQUAL(1, error_count);

	mock().expectNCalls(2, "disable");
	mock().expectNCalls(2, "destroy");
}

TEST(SafetyTestGroup, AllEntriesFailCheck) {
	struct safety_entry* entry1 = allocate_mock_entry();
	struct safety_entry* entry2 = allocate_mock_entry();
	safety_add(ctx, entry1);
	safety_add(ctx, entry2);

	mock().expectNCalls(2, "check").andReturnValue(SAFETY_STATUS_STALE);

	int error_count = 0;
	LONGS_EQUAL(2, safety_check(ctx, [](struct safety_entry*, safety_entry_status_t, void* t) {
		int* c = static_cast<int*>(t);
		(*c)++;
	}, &error_count));
	LONGS_EQUAL(2, error_count);

	mock().expectNCalls(2, "disable");
	mock().expectNCalls(2, "destroy");
}

TEST(SafetyTestGroup, AddAndEnableSuccess) {
	struct safety_entry* entry = allocate_mock_entry();
	mock().expectOneCall("enable").andReturnValue(0);
	LONGS_EQUAL(0, safety_add_and_enable(ctx, entry));
	mock().expectOneCall("disable");
	mock().expectOneCall("destroy");
}

TEST(SafetyTestGroup, AddAndEnableFailsToEnableRollsBack) {
	struct safety_entry* entry = allocate_mock_entry();
	mock().expectOneCall("enable").andReturnValue(-1);
	LONGS_EQUAL(-EIO, safety_add_and_enable(ctx, entry));
	// entry는 safety context에 남지 않아야 하므로 teardown 시 destroy 안 됨
	free_mock_entry(entry);
}

TEST(SafetyTestGroup, AddAndEnableFailsOnAdd) {
	LONGS_EQUAL(-EINVAL, safety_add_and_enable(ctx, nullptr));
}
