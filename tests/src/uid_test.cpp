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
#include "fs/fs.h"
#include <string.h>
#include <time.h>

#define CACHE_CAPACITY	1024
#define ENTRY_SIZE	54

struct fs {
	struct fs_api api;
};

static int fake_read(struct fs *self, const char *filepath,
		const size_t offset, void *buf, const size_t bufsize) {
	return mock().actualCall(__func__)
		.withStringParameter("filepath", filepath)
		.withParameter("offset", offset)
		.withOutputParameter("buf", buf)
		.returnIntValue();
}
static int fake_append(struct fs *self, const char *filepath,
		const void *data, const size_t datasize) {
	return mock().actualCall(__func__)
		.withStringParameter("filepath", filepath)
		.withMemoryBufferParameter("data", (const uint8_t *)data, datasize)
		.returnIntValue();
}
static int fake_size(struct fs *self, const char *filepath, size_t *size) {
	return mock().actualCall(__func__)
		.withStringParameter("filepath", filepath)
		.withOutputParameter("size", size)
		.returnIntValue();
}
static int fake_write(struct fs *self, const char *filepath,
		const size_t offset, const void *data, const size_t datasize) {
	return mock().actualCall(__func__)
		.withStringParameter("filepath", filepath)
		.withParameter("offset", offset)
		.withMemoryBufferParameter("data", (const uint8_t *)data, datasize)
		.returnIntValue();
}

static void on_uid_update(const uid_id_t id, uid_status_t status,
		time_t expiry, void *ctx) {
	mock().actualCall(__func__)
		.withMemoryBufferParameter("id", id, sizeof(uid_id_t))
		.withParameter("status", status)
		.withParameter("expiry", expiry)
		.withParameter("ctx", ctx);
}

static time_t mock_time = 0;

time_t time(time_t *tloc) {
	if (tloc != NULL) {
		*tloc = mock_time;
	}
	return mock_time;
}

TEST_GROUP(UID) {
	struct uid_store *cache;
	struct uid_store_config config;
	struct fs fs;
	uid_id_t pid;
	uid_id_t id1, id2;

	void setup() {
		fs = (struct fs) {
			.api = {
				.read = fake_read,
				.write = fake_write,
				.append = fake_append,
				.size = fake_size,
			},
		};
		config = (struct uid_store_config) {
			.fs = &fs,
			.ns = "cache",
			.capacity = CACHE_CAPACITY,
		};
		cache = uid_store_create(&config);

		memset(id1, 0x01, sizeof(uid_id_t));
		memset(id2, 0x02, sizeof(uid_id_t));
		memset(pid, 0xA0, sizeof(uid_id_t));
	}

	void teardown() {
		uid_store_destroy(cache);

		mock().checkExpectations();
		mock().clear();
	}

	void expect_size(const char *filepath, const size_t *size) {
		if (*size <= 0) {
			mock().expectOneCall("fake_size")
				.withStringParameter("filepath", filepath)
				.ignoreOtherParameters()
				.andReturnValue((int)*size);
		} else {
			mock().expectOneCall("fake_size")
				.withStringParameter("filepath", filepath)
				.withOutputParameterReturning("size", size, sizeof(size_t))
				.andReturnValue((int)*size);
		}
	}
	void expect_append(const char *filepath,
			const void *data, const size_t *datasize) {
		if (data) {
			mock().expectOneCall("fake_append")
				.withStringParameter("filepath", filepath)
				.withMemoryBufferParameter("data", (const uint8_t *)data, *datasize)
				.andReturnValue((int)*datasize);
		} else {
			mock().expectOneCall("fake_append")
				.withStringParameter("filepath", filepath)
				.ignoreOtherParameters()
				.andReturnValue(ENTRY_SIZE);
		}
	}
	void expect_read(const char *filepath, const void *data,
			const size_t *datasize, const size_t *offset) {
		if (data && offset) {
			mock().expectOneCall("fake_read")
				.withStringParameter("filepath", filepath)
				.withParameter("offset", *offset)
				.withOutputParameterReturning("buf", data, *datasize)
				.andReturnValue((int)*datasize);
		} else if (data) {
			mock().expectOneCall("fake_read")
				.withStringParameter("filepath", filepath)
				.withParameter("offset", 0)
				.withOutputParameterReturning("buf", data, *datasize)
				.andReturnValue((int)*datasize);
		}
	}
	void expect_write(const char *filepath, const uint8_t *data,
			const size_t *datasize, const size_t *offset) {
		if (data && offset) {
			mock().expectOneCall("fake_write")
				.withStringParameter("filepath", filepath)
				.withParameter("offset", *offset)
				.withMemoryBufferParameter("data", data, *datasize)
				.andReturnValue((int)*datasize);
		} else if (data) {
			mock().expectOneCall("fake_write")
				.withStringParameter("filepath", filepath)
				.withParameter("offset", 0)
				.withMemoryBufferParameter("data", data, *datasize)
				.andReturnValue((int)*datasize);
		}
	}
};

TEST(UID, ShouldReturnNoEntry_WhenCacheIsEmpty) {
	size_t size = (size_t)-ENOENT;
	expect_size("uid/cache/01/01.bin", &size);
	LONGS_EQUAL(UID_STATUS_NO_ENTRY, uid_status(cache, id1, NULL, NULL));
}

TEST(UID, ShouldReturnAccepted_WhenUIDAccepted) {
	const uint8_t expected[] = {
		0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
		0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
		0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,
		0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,
		0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,
		0x00,0x00
	};
	size_t size[] = { (size_t)-ENOENT, sizeof(expected) };

	expect_size("uid/cache/01/01.bin", &size[0]);
	expect_append("uid/cache/01/01.bin", expected, &size[1]);

	uid_update(cache, id1, pid, UID_STATUS_ACCEPTED, mock_time + 60);

	time_t expiry = 0;
	uid_status_t status = uid_status(cache, id1, NULL, &expiry);

	LONGS_EQUAL(UID_STATUS_ACCEPTED, status);
	LONGS_EQUAL(mock_time + 60, expiry);
}

TEST(UID, ShouldOverwriteStatus_WhenUIDUpdated) {
	const uint8_t expected[] = {
		0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
		0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
		0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,
		0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,
		0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,
		0x00,0x00
	};
	const uint8_t expected2[] = {
		0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
		0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
		0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,
		0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,
		0x78,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,
		0x00,0x00
	};
	size_t size[] = { (size_t)-ENOENT, sizeof(expected) };

	expect_size("uid/cache/01/01.bin", &size[0]);
	expect_append("uid/cache/01/01.bin", expected, &size[1]);
	uid_update(cache, id1, pid, UID_STATUS_ACCEPTED, mock_time + 60);

	expect_size("uid/cache/01/01.bin", &size[1]);
	expect_read("uid/cache/01/01.bin", expected, &size[1], NULL);
	expect_write("uid/cache/01/01.bin", expected2, &size[1], NULL);
	uid_update(cache, id1, pid, UID_STATUS_BLOCKED, mock_time + 120);

	time_t expiry = 0;
	uid_status_t status = uid_status(cache, id1, NULL, &expiry);

	LONGS_EQUAL(UID_STATUS_BLOCKED, status);
	LONGS_EQUAL(mock_time + 120, expiry);
}

TEST(UID, ShouldNotCollide_WhenDifferentUIDsUpdated) {
	const size_t size = (size_t)-ENOENT;
	expect_size("uid/cache/01/01.bin", &size);
	expect_size("uid/cache/02/02.bin", &size);
	expect_append("uid/cache/01/01.bin", NULL, NULL);
	expect_append("uid/cache/02/02.bin", NULL, NULL);

	uid_update(cache, id1, pid, UID_STATUS_ACCEPTED, mock_time + 60);
	uid_update(cache, id2, pid, UID_STATUS_EXPIRED, mock_time + 30);

	time_t expiry1 = 0, expiry2 = 0;
	uid_status_t status1 = uid_status(cache, id1, NULL, &expiry1);
	uid_status_t status2 = uid_status(cache, id2, NULL, &expiry2);

	LONGS_EQUAL(UID_STATUS_ACCEPTED, status1);
	LONGS_EQUAL(UID_STATUS_EXPIRED, status2);
	LONGS_EQUAL(mock_time + 60, expiry1);
	LONGS_EQUAL(mock_time + 30, expiry2);
}

TEST(UID, ShouldReturnNoEntry_WhenUIDNotFoundDeleting) {
	const size_t size = (size_t)-ENOENT;
	expect_size("uid/cache/01/01.bin", &size);
	LONGS_EQUAL(-ENOENT, uid_delete(cache, id1));
}

TEST(UID, ShouldRunCallback_WhenUIDUpdated) {
	size_t size = (size_t)-ENOENT;
	uid_register_update_cb(cache, on_uid_update, NULL);

	expect_size("uid/cache/01/01.bin", &size);
	expect_append("uid/cache/01/01.bin", NULL, NULL);

	mock().expectOneCall("on_uid_update")
		.withMemoryBufferParameter("id", id1, sizeof(uid_id_t))
		.withParameter("status", UID_STATUS_ACCEPTED)
		.withParameter("expiry", mock_time + 10)
		.withParameter("ctx", (void *)0);

	uid_update(cache, id1, pid, UID_STATUS_ACCEPTED, mock_time + 10);
}

TEST(UID, ShouldDeleteUIDBothCacheAndFlash_WhenUIDDeleted) {
	const uint8_t dummy[] = {
		0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
		0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
		0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,
		0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,
		0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,
		0x00,0x00,
	};
	const uint8_t expected[] = {
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,
	};
	const size_t size[] = { (size_t)-ENOENT, sizeof(dummy) };

	expect_size("uid/cache/01/01.bin", &size[0]);
	expect_append("uid/cache/01/01.bin", NULL, NULL);
	uid_update(cache, id1, pid, UID_STATUS_ACCEPTED, mock_time + 60);

	expect_size("uid/cache/01/01.bin", &size[1]);
	expect_read("uid/cache/01/01.bin", dummy, &size[1], NULL);
	expect_write("uid/cache/01/01.bin", expected, &size[1], NULL);
	LONGS_EQUAL(0, uid_delete(cache, id1));

	expect_size("uid/cache/01/01.bin", &size[0]);
	LONGS_EQUAL(UID_STATUS_NO_ENTRY, uid_status(cache, id1, NULL, NULL));
}

TEST(UID, ShouldReturnNoEntry_WhenNullIDGiven) {
	LONGS_EQUAL(UID_STATUS_NO_ENTRY, uid_status(cache, NULL, NULL, NULL));
}

TEST(UID, ShouldFailUpdate_WhenNullStoreGiven) {
	LONGS_EQUAL(-EINVAL, uid_update(NULL, id1, pid, UID_STATUS_ACCEPTED, 10));
}

TEST(UID, ShouldReturnNullStore_WhenConfigInvalid) {
	struct uid_store_config c = {0};
	POINTERS_EQUAL(NULL, uid_store_create(&c));
}

TEST(UID, ShouldNotCrash_WhenDeleteWithNullID) {
	LONGS_EQUAL(-EINVAL, uid_delete(cache, NULL));
}

TEST(UID, ShouldAllowOverwrite_WhenHashConflicts) {
	// Two IDs with the same hash slot (manually crafted)
	uid_id_t idA, idB;
	memset(idA, 0x11, sizeof(uid_id_t));
	memset(idB, 0x11, sizeof(uid_id_t));
	idB[sizeof(uid_id_t)-1] ^= 0xFF;

	size_t size = (size_t)-ENOENT;
	expect_size("uid/cache/11/11.bin", &size);
	expect_append("uid/cache/11/11.bin", NULL, NULL);
	uid_update(cache, idA, pid, UID_STATUS_ACCEPTED, 5);

	expect_size("uid/cache/11/11.bin", &size);
	expect_append("uid/cache/11/11.bin", NULL, NULL);
	uid_update(cache, idB, pid, UID_STATUS_BLOCKED, 6);

	// Ensure both are correctly returned (one from cache, one from disk)
	time_t expiry = 0;
	uid_status_t status = uid_status(cache, idA, NULL, &expiry);
	LONGS_EQUAL(UID_STATUS_ACCEPTED, status);
	LONGS_EQUAL(5, expiry);
}

TEST(UID, ShouldNotFail_WhenPIDisNull) {
	size_t size = (size_t)-ENOENT;
	expect_size("uid/cache/01/01.bin", &size);
	expect_append("uid/cache/01/01.bin", NULL, NULL);

	LONGS_EQUAL(0, uid_update(cache, id1, NULL, UID_STATUS_EXPIRED, 99));
}

TEST(UID, ShouldAllowMultipleStores_IsolatedBehavior) {
	struct uid_store_config c2 = config;
	c2.ns = "other";
	struct uid_store *other = uid_store_create(&c2);
	uid_id_t id_other;
	memset(id_other, 0x30, sizeof(uid_id_t));

	size_t size = (size_t)-ENOENT;
	expect_size("uid/cache/01/01.bin", &size);
	expect_append("uid/cache/01/01.bin", NULL, NULL);
	uid_update(cache, id1, pid, UID_STATUS_ACCEPTED, 1);

	expect_size("uid/other/30/30.bin", &size);
	expect_append("uid/other/30/30.bin", NULL, NULL);
	uid_update(other, id_other, pid, UID_STATUS_BLOCKED, 2);

	time_t exp1 = 0, exp2 = 0;
	LONGS_EQUAL(UID_STATUS_ACCEPTED, uid_status(cache, id1, NULL, &exp1));
	LONGS_EQUAL(UID_STATUS_BLOCKED, uid_status(other, id_other, NULL, &exp2));

	uid_store_destroy(other);
}

TEST(UID, ShouldOverwriteSecondEntry_WhenUIDUpdatedWithMiddleOffset) {
	uid_id_t id3;
	memset(id3, 0x03, sizeof(uid_id_t));

	const size_t file_size = ENTRY_SIZE * 3;
	const size_t offset_second = ENTRY_SIZE * 1;
	const size_t size[] = { ENTRY_SIZE, (size_t)-ENOENT };

	const uint8_t empty_entry[ENTRY_SIZE] = { 0 };
	uint8_t second_entry[ENTRY_SIZE] = { 0 };
	memset(second_entry, 0x02, sizeof(uid_id_t)); // id2
	memset(&second_entry[21], 0xA0, sizeof(uid_id_t)); // pid
	*((uint64_t *)(void *)&second_entry[42]) = 0x3C; // expiry
	second_entry[50] = UID_STATUS_ACCEPTED;

	uint8_t updated_entry[ENTRY_SIZE];
	memcpy(updated_entry, second_entry, sizeof(updated_entry));
	*((uint64_t *)(void *)&updated_entry[42]) = 0x78; // new expiry
	updated_entry[50] = UID_STATUS_BLOCKED;

	// 1. append id1
	expect_size("uid/cache/01/01.bin", &size[1]);
	expect_append("uid/cache/01/01.bin", NULL, NULL);
	uid_update(cache, id1, pid, UID_STATUS_ACCEPTED, 1);

	// 2. append id2
	expect_size("uid/cache/02/02.bin", &size[1]);
	expect_append("uid/cache/02/02.bin", NULL, NULL);
	uid_update(cache, id2, pid, UID_STATUS_ACCEPTED, 2);

	// 3. append id3
	expect_size("uid/cache/03/03.bin", &size[1]);
	expect_append("uid/cache/03/03.bin", NULL, NULL);
	uid_update(cache, id3, pid, UID_STATUS_ACCEPTED, 3);

	// 4. overwrite id2 → simulate entry found at offset 1
	expect_size("uid/cache/02/02.bin", &file_size);
	expect_read("uid/cache/02/02.bin", empty_entry, &size[0], NULL);
	expect_read("uid/cache/02/02.bin", second_entry, &size[0], &offset_second);
	expect_write("uid/cache/02/02.bin", updated_entry, &size[0], &offset_second);

	uid_update(cache, id2, pid, UID_STATUS_BLOCKED, 120);

	time_t expiry = 0;
	uid_status_t status = uid_status(cache, id2, NULL, &expiry);

	LONGS_EQUAL(UID_STATUS_BLOCKED, status);
	LONGS_EQUAL(120, expiry);
}

TEST(UID, ShouldSkipZeroedEntry_WhenSearchingInFile) {
	const size_t file_size = ENTRY_SIZE * 2;
	const size_t offset0 = 0;
	const size_t offset1 = ENTRY_SIZE;
	const size_t size = ENTRY_SIZE;

	uid_id_t id;
	memset(id, 0x02, sizeof(uid_id_t));

	uint8_t zeroed[ENTRY_SIZE] = { 0 };
	uint8_t valid[ENTRY_SIZE] = { 0 };
	memcpy(valid, id2, sizeof(uid_id_t));
	memcpy(&valid[21], pid, sizeof(uid_id_t));
	*((uint64_t *)(void *)&valid[42]) = 123;
	valid[50] = UID_STATUS_ACCEPTED;

	expect_size("uid/cache/02/02.bin", &file_size);
	expect_read("uid/cache/02/02.bin", zeroed, &size, &offset0);
	expect_read("uid/cache/02/02.bin", valid, &size, &offset1);

	time_t expiry = 0;
	uid_status_t status = uid_status(cache, id2, NULL, &expiry);

	LONGS_EQUAL(UID_STATUS_ACCEPTED, status);
	LONGS_EQUAL(123, expiry);
}

TEST(UID, ShouldHandleMultipleZeroedEntriesBeforeValidOne) {
	const uint8_t zeroed[ENTRY_SIZE] = {0};
	uint8_t valid[ENTRY_SIZE];
	memset(valid, 0x03, sizeof(uid_id_t));
	memset(&valid[21], 0xA0, sizeof(uid_id_t));
	*((uint64_t *)(void *)&valid[42]) = 789;
	*((uint32_t *)(void *)&valid[50]) = UID_STATUS_EXPIRED;

	const size_t size = ENTRY_SIZE;
	const size_t off[] = { 0, ENTRY_SIZE, ENTRY_SIZE*2, ENTRY_SIZE*3 };
	const size_t file_size = ENTRY_SIZE * 4;

	expect_size("uid/cache/03/03.bin", &file_size);
	expect_read("uid/cache/03/03.bin", zeroed, &size, &off[0]);
	expect_read("uid/cache/03/03.bin", zeroed, &size, &off[1]);
	expect_read("uid/cache/03/03.bin", zeroed, &size, &off[2]);
	expect_read("uid/cache/03/03.bin", valid, &size, &off[3]);

	time_t expiry = 0;
	uid_id_t id3;
	memset(id3, 0x03, sizeof(uid_id_t));
	uid_status_t status = uid_status(cache, id3, NULL, &expiry);

	LONGS_EQUAL(UID_STATUS_EXPIRED, status);
	LONGS_EQUAL(789, expiry);
}

TEST(UID, ShouldUpdateWithoutCallback_WhenCallbackNotRegistered) {
	size_t size = (size_t)-ENOENT;
	expect_size("uid/cache/01/01.bin", &size);
	expect_append("uid/cache/01/01.bin", NULL, NULL);

	LONGS_EQUAL(0, uid_update(cache, id1, pid, UID_STATUS_ACCEPTED, 10));
}

TEST(UID, ShouldReturnNoEntry_WhenFileContainsOnlyZeroedRecords) {
	const size_t file_size = ENTRY_SIZE * 2;
	const uint8_t zero[ENTRY_SIZE] = { 0 };
	const size_t size = ENTRY_SIZE;
	const size_t off[] = { 0, ENTRY_SIZE };

	expect_size("uid/cache/01/01.bin", &file_size);
	expect_read("uid/cache/01/01.bin", zero, &size, &off[0]);
	expect_read("uid/cache/01/01.bin", zero, &size, &off[1]);

	LONGS_EQUAL(UID_STATUS_NO_ENTRY, uid_status(cache, id1, NULL, NULL));
}
