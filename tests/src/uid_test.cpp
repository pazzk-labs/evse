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

#include "uid.h"
#include <string.h>
#include <time.h>

#define CACHE_CAPACITY 1024

static void on_uid_update(const uid_id_t id, uid_status_t status,
		time_t expiry, void *ctx) {
	mock().actualCall(__func__)
		.withMemoryBufferParameter("id", id, sizeof(uid_id_t))
		.withParameter("status", status)
		.withParameter("expiry", expiry)
		.withParameter("ctx", ctx);
}

TEST_GROUP(UID) {
	uid_id_t id1, id2;
	uid_id_t pid;
	time_t now;

	void setup() {
		memset(id1, 0x01, sizeof(uid_id_t));
		memset(id2, 0x02, sizeof(uid_id_t));
		memset(pid, 0xA0, sizeof(uid_id_t));
		now = time(NULL);
		uid_init(NULL, CACHE_CAPACITY);
	}

	void teardown() {
		uid_deinit();
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(UID, EmptyCache_ShouldReturnNoEntry) {
	LONGS_EQUAL(UID_STATUS_NO_ENTRY, uid_status(id1, NULL));
}

TEST(UID, InsertAndRetrieveUID) {
	uid_update_cache(id1, pid, UID_STATUS_ACCEPTED, now + 60);
	time_t expiry = 0;
	uid_status_t status = uid_status(id1, &expiry);

	LONGS_EQUAL(UID_STATUS_ACCEPTED, status);
	LONGS_EQUAL(now + 60, expiry);
}

TEST(UID, OverwriteUIDEntry) {
	uid_update_cache(id1, pid, UID_STATUS_ACCEPTED, now + 60);
	uid_update_cache(id1, pid, UID_STATUS_BLOCKED, now + 120);

	time_t expiry = 0;
	uid_status_t status = uid_status(id1, &expiry);

	LONGS_EQUAL(UID_STATUS_BLOCKED, status);
	LONGS_EQUAL(now + 120, expiry);
}

TEST(UID, DifferentUIDsShouldNotCollide) {
	uid_update_cache(id1, pid, UID_STATUS_ACCEPTED, now + 60);
	uid_update_cache(id2, pid, UID_STATUS_EXPIRED, now + 30);

	time_t expiry1 = 0, expiry2 = 0;
	uid_status_t status1 = uid_status(id1, &expiry1);
	uid_status_t status2 = uid_status(id2, &expiry2);

	LONGS_EQUAL(UID_STATUS_ACCEPTED, status1);
	LONGS_EQUAL(UID_STATUS_EXPIRED, status2);
	LONGS_EQUAL(now + 60, expiry1);
	LONGS_EQUAL(now + 30, expiry2);
}

TEST(UID, DeleteUID_ShouldRemoveFromCache) {
	uid_update_cache(id1, pid, UID_STATUS_ACCEPTED, now + 60);
	LONGS_EQUAL(0, uid_delete(id1));
	LONGS_EQUAL(UID_STATUS_NO_ENTRY, uid_status(id1, NULL));
}

TEST(UID, DeleteUID_NotFound) {
	LONGS_EQUAL(-ENOENT, uid_delete(id1));
}

TEST(UID, UpdateCallbackIsCalled) {
	uid_register_update_cb(on_uid_update, NULL);

	mock().expectOneCall("on_uid_update")
		.withMemoryBufferParameter("id", id1, sizeof(uid_id_t))
		.withParameter("status", UID_STATUS_ACCEPTED)
		.withParameter("expiry", now + 10)
		.withParameter("ctx", (void *)0);

	uid_update_cache(id1, pid, UID_STATUS_ACCEPTED, now + 10);
}
