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

#include "fs/logfs.h"

#define BASE_PATH		"logfs"
#define CACHE_SIZE		16
#define MAX_SIZE		128

struct fs {
	struct fs_api api;
};

static int log_count;

static int fake_read(struct fs *self, const char *filepath,
		const size_t offset, void *buf, const size_t bufsize) {
	return mock().actualCall(__func__)
		.withParameter("filepath", filepath)
		.withParameter("offset", offset)
		.withOutputParameter("buf", buf)
		.returnIntValue();
}

static int fake_append(struct fs *self, const char *filepath,
		const void *data, const size_t datasize) {
	return mock().actualCall(__func__)
		.withParameter("filepath", filepath)
		.withParameter("datasize", datasize)
		.returnIntValue();
}

static int fake_erase(struct fs *self, const char *filepath) {
	return mock().actualCall(__func__)
		.withParameter("filepath", filepath)
		.returnIntValue();
}

static int fake_size(struct fs *self, const char *filepath, size_t *size) {
	return mock().actualCall(__func__)
		.withParameter("filepath", filepath)
		.withOutputParameter("size", size)
		.returnIntValue();
}
static int fake_dir(struct fs *self, const char *path,
		fs_dir_cb_t cb, void *cb_ctx) {
	int err = mock().actualCall(__func__)
		.withParameter("path", path)
		.returnIntValue();

	if (err >= 0) {
		for (int i = 0; i < log_count; i++) {
			char filename[FS_FILENAME_MAX+1];
			snprintf(filename, sizeof(filename)-1, "%d", i+1);
			(*cb)(self, FS_FILE_TYPE_FILE, filename, cb_ctx);
		}
	}

	return err;
}

static int fake_usage(struct fs *self, size_t *used, size_t *total) {
	return mock().actualCall(__func__)
		.withOutputParameter("used", used)
		.withOutputParameter("total", total)
		.returnIntValue();
}
struct fs *fs_create(struct flash *flash) {
	static struct fs fs = {
		.api = {
			.read = fake_read,
			.append = fake_append,
			.erase = fake_erase,
			.size = fake_size,
			.dir = fake_dir,
			.usage = fake_usage,
		},
	};
	return &fs;
}
void fs_destroy(struct fs *fs) {
	return;
}

static void on_dir(struct logfs *fs, const time_t timestamp, void *ctx) {
	mock().actualCall("on_dir").withParameter("timestamp", timestamp);
}

TEST_GROUP(logfs) {
	struct logfs *logfs;
	struct fs *fs;
	char filepath[FS_FILENAME_MAX+1];

	void setup(void) {
		log_count = 0;
		fs = fs_create(0);
		logfs = logfs_create(fs, BASE_PATH, MAX_SIZE, 10, CACHE_SIZE);
	}
	void teardown(void) {
		mock().checkExpectations();
		mock().clear();

		logfs_destroy(logfs);
		fs_destroy(fs);
	}

	void flush_logs(int n, size_t dir_size, size_t file_size) {
		log_count = n;
		size_t zero_size = 0;
		char files[n][FS_FILENAME_MAX+1];

		mock().expectOneCall("fake_dir")
			.withParameter("path", BASE_PATH)
			.andReturnValue(0);
		for (int i = 0; i < n; i++) {
			snprintf(files[i], sizeof(files[i])-1, "%s/%d", BASE_PATH, i+1);
			mock().expectOneCall("fake_size")
				.withParameter("filepath", files[i])
				.withOutputParameterReturning("size", &file_size, sizeof(size_t))
				.andReturnValue(0);
		}
		mock().expectOneCall("fake_size")
			.withParameter("filepath", "logfs/0")
			.withOutputParameterReturning("size", &zero_size, sizeof(size_t))
			.andReturnValue(0);

		logfs_flush(logfs);
	}
	void expect_reclaim(const char *oldest, const char *current, size_t *dir_size, size_t *file_size) {
		mock().expectOneCall("fake_erase")
			.withParameter("filepath", oldest)
			.andReturnValue(0);
		mock().expectOneCall("fake_size") // exsiting log
			.withParameter("filepath", current)
			.withOutputParameterReturning("size", file_size, sizeof(size_t))
			.andReturnValue(0);
	}
	void expect_reclaim_by_size(const char *oldest, const char *current,
			size_t *dir_size, size_t *post_dir_size, size_t *file_size) {
		mock().expectOneCall("fake_erase")
			.withParameter("filepath", oldest)
			.andReturnValue(0);
		mock().expectOneCall("fake_size") // exsiting log
			.withParameter("filepath", current)
			.withOutputParameterReturning("size", file_size, sizeof(size_t))
			.andReturnValue(0);
	}
	void expect_no_reclaim(const char *current, size_t *dir_size, size_t *file_size) {
		mock().expectOneCall("fake_size") // exsiting log
			.withParameter("filepath", current)
			.withOutputParameterReturning("size", file_size, sizeof(size_t))
			.andReturnValue(0);
	}
	void expect_fetch(const char *current, size_t *current_size, size_t *dir_size) {
		mock().expectOneCall("fake_dir")
			.withParameter("path", BASE_PATH)
			.andReturnValue(0);
		mock().expectOneCall("fake_size")
			.withParameter("filepath", current)
			.withOutputParameterReturning("size", current_size, sizeof(size_t))
			.andReturnValue(0);
	}
	void get_filepath(time_t ts) {
		snprintf(filepath, sizeof(filepath)-1, "%s/%ld", BASE_PATH, ts);
	}
};

TEST(logfs, flush_ShouldFetchLogsFromFS_WhenCalledFirstTime) {
	flush_logs(10, 128, 4);
}
TEST(logfs, dir_ShouldListAllLogs_WhenMaxFilesIsZero) {
	int n = 10;
	flush_logs(n, 128, 4);

	for (int i = 0; i < n; i++) {
		mock().expectOneCall("on_dir").withParameter("timestamp", i+1);
	}

	logfs_dir(logfs, on_dir, 0, 0);
}
TEST(logfs, dir_ShouldListMaxFiles_WhenMaxFilesIsSet) {
	int n = 10;
	int max_files = 5;
	flush_logs(n, 128, 4);

	for (int i = 0; i < max_files; i++) {
		mock().expectOneCall("on_dir").withParameter("timestamp", i+1);
	}

	logfs_dir(logfs, on_dir, 0, (size_t)max_files);
}
TEST(logfs, size_ShouldReturnFileSize_WhenTimestampGiven) {
	flush_logs(10, 128, 4);
	LONGS_EQUAL(4, logfs_size(logfs, 5));
}
TEST(logfs, size_ShouldReturnZero_WhenTimestampNotFound) {
	flush_logs(10, 128, 4);
	LONGS_EQUAL(0, logfs_size(logfs, 11));
}
TEST(logfs, size_ShouldReturnZero_WhenNoLogs) {
	LONGS_EQUAL(0, logfs_size(logfs, 1));
}
TEST(logfs, size_ShouldReturnAllUsedSpace_WhenNoTimestampGiven) {
	size_t dir_size = 40;
	flush_logs(10, 128, 4);
	LONGS_EQUAL(dir_size, logfs_size(logfs, 0));
}
TEST(logfs, delete_ShouldDeleteLog_WhenTimestampGiven) {
	flush_logs(10, 128, 4);
	mock().expectOneCall("fake_erase")
		.withParameter("filepath", "logfs/5")
		.andReturnValue(0);
	LONGS_EQUAL(0, logfs_delete(logfs, 5));
}
TEST(logfs, delete_ShouldReturnError_WhenTimestampNotFound) {
	flush_logs(10, 128, 4);
	mock().expectOneCall("fake_erase")
		.withParameter("filepath", "logfs/11")
		.andReturnValue(-ENOENT);
	LONGS_EQUAL(-ENOENT, logfs_delete(logfs, 11));
}
TEST(logfs, clear_ShouldDeleteAllLogs) {
	char files[10][FS_FILENAME_MAX+1];
	flush_logs(10, 128, 4);

	for (int i = 0; i < 10; i++) {
		snprintf(files[i], sizeof(files[i])-1, "%s/%d", BASE_PATH, i+1);
		mock().expectOneCall("fake_erase")
			.withParameter("filepath", files[i])
			.andReturnValue(0);
	}

	LONGS_EQUAL(0, logfs_clear(logfs));
}
TEST(logfs, read_ShouldReadLog_WhenTimestampGiven) {
	flush_logs(10, 128, 4);
	char buf[4];
	mock().expectOneCall("fake_read")
		.withParameter("filepath", "logfs/5")
		.withParameter("offset", 0)
		.withOutputParameterReturning("buf", buf, 4)
		.andReturnValue(4);
	LONGS_EQUAL(4, logfs_read(logfs, 5, 0, buf, 4));
}
TEST(logfs, write_ShouldWriteInCache_WhenLogSizeIsSmallerThanOrEqualToCacheSize) {
	LONGS_EQUAL(4, logfs_write(logfs, 0, "log1", 4));
	LONGS_EQUAL(12, logfs_write(logfs, 0, "log1log1log1", 12));
}
TEST(logfs, write_ShouldFlushCache_WhenLogSizeIsLargerThanCacheSize) {
	size_t dir_size = 0;
	size_t file_size = 16;
	char buf[17];

	mock().expectOneCall("fake_dir")
		.withParameter("path", BASE_PATH)
		.andReturnValue(-ENOENT);
	expect_no_reclaim("logfs/0", &dir_size, &file_size);
	mock().expectOneCall("fake_append")
		.withParameter("filepath", "logfs/0")
		.withParameter("datasize", 16)
		.andReturnValue(16);
	LONGS_EQUAL(17, logfs_write(logfs, 0, buf, 17));
}
TEST(logfs, write_ShouldInitializeLogListFirst_WhenLogListIsEmptyWhileFSIsNotEmpty) {
	size_t size = 10;
	time_t ts = 1;
	log_count = 1;
	snprintf(filepath, sizeof(filepath)-1, "%s/%ld", BASE_PATH, ts);

	expect_fetch("logfs/0", &size, &size);
	mock().expectOneCall("fake_size")
		.withParameter("filepath", filepath)
		.withOutputParameterReturning("size", &size, sizeof(size_t))
		.andReturnValue(0);

	logfs_write(logfs, ts, "log1", 4);
}
TEST(logfs, write_ShouldNotInitializeLogList_WhenLogListIsNotEmpty) {
	size_t size = 10;
	time_t ts = 1;
	log_count = 1;
	snprintf(filepath, sizeof(filepath)-1, "%s/%ld", BASE_PATH, ts);

	expect_fetch("logfs/0", &size, &size);
	mock().expectOneCall("fake_size")
		.withParameter("filepath", filepath)
		.withOutputParameterReturning("size", &size, sizeof(size_t))
		.andReturnValue(0);

	logfs_write(logfs, ts, "log1", 4);
	logfs_write(logfs, ts, "log1", 4);
}
TEST(logfs, write_ShouldReclaimOldest_WhenMoreThanMaxLogsGiven) {
	flush_logs(10, 128, 4);

	size_t dir_size = 40;
	size_t file_size = 8;
	size_t zero_size = 0;
	char buf[file_size];

	expect_reclaim("logfs/1", "logfs/0", &dir_size, &zero_size);
	LONGS_EQUAL(file_size, logfs_write(logfs, 12, buf, file_size));

	expect_reclaim("logfs/2", "logfs/12", &dir_size, &file_size);
	mock().expectOneCall("fake_append")
		.withParameter("filepath", "logfs/12")
		.withParameter("datasize", 8)
		.andReturnValue(8);
	LONGS_EQUAL(file_size, logfs_write(logfs, 13, buf, file_size));

	expect_no_reclaim("logfs/13", &dir_size, &file_size);
	mock().expectOneCall("fake_append")
		.withParameter("filepath", "logfs/13")
		.withParameter("datasize", 8)
		.andReturnValue(8);
	LONGS_EQUAL(file_size, logfs_write(logfs, 14, buf, file_size));
}
TEST(logfs, write_ShouldRepeatReclaim_WhenMuchLargerThanCacheGiven) {
	char buf[CACHE_SIZE*4];
	size_t dirsize_1 = 0;
	size_t dirsize_2 = CACHE_SIZE;
	size_t dirsize_3 = CACHE_SIZE*2;
	size_t dirsize_4 = CACHE_SIZE*3;

	mock().expectNCalls(2, "fake_dir")
		.withParameter("path", BASE_PATH)
		.andReturnValue(-ENOENT);
	expect_no_reclaim("logfs/0", &dirsize_1, &dirsize_1);
	expect_no_reclaim("logfs/1", &dirsize_1, &dirsize_2);
	mock().expectOneCall("fake_append")
		.withParameter("filepath", "logfs/1")
		.withParameter("datasize", CACHE_SIZE)
		.andReturnValue(CACHE_SIZE);

	expect_no_reclaim("logfs/1", &dirsize_2, &dirsize_3);
	mock().expectOneCall("fake_append")
		.withParameter("filepath", "logfs/1")
		.withParameter("datasize", CACHE_SIZE)
		.andReturnValue(CACHE_SIZE);

	expect_no_reclaim("logfs/1", &dirsize_3, &dirsize_4);
	mock().expectOneCall("fake_append")
		.withParameter("filepath", "logfs/1")
		.withParameter("datasize", CACHE_SIZE)
		.andReturnValue(CACHE_SIZE);

	logfs_write(logfs, 1, buf, sizeof(buf));
}
TEST(logfs, write_ShouldReclaimOldest_WhenMoreThanMaxSizeGiven) {
	flush_logs(8, 128, 16);

	size_t dir_size = 128;
	size_t file_size = 0;
	size_t dir_size2 = dir_size - 16;
	char buf[16];

	expect_reclaim_by_size("logfs/1", "logfs/0", &dir_size, &dir_size2, &file_size);

	logfs_write(logfs, 10, buf, sizeof(buf));
}
TEST(logfs, write_ShouldReclaimMultiple_WhenMoreThanMaxSizeGiven) {
	flush_logs(8, 128, 16);

	size_t dir_size = 128;
	size_t file_size = 16;
	size_t zero_size = 0;
	size_t dir_size2 = dir_size - 16;
	size_t dir_size3 = dir_size2 - 16;
	size_t dir_size4 = dir_size3 - 16;
	size_t dir_size5 = dir_size4 + 16;
	char buf[48];

	mock().expectOneCall("fake_erase")
		.withParameter("filepath", "logfs/1")
		.andReturnValue(0);
	mock().expectOneCall("fake_erase")
		.withParameter("filepath", "logfs/2")
		.andReturnValue(0);
	mock().expectOneCall("fake_erase")
		.withParameter("filepath", "logfs/3")
		.andReturnValue(0);
	mock().expectOneCall("fake_size")
		.withParameter("filepath", "logfs/0")
		.withOutputParameterReturning("size", &zero_size, sizeof(size_t))
		.andReturnValue(0);

	expect_no_reclaim("logfs/10", &dir_size4, &file_size);
	mock().expectOneCall("fake_append")
		.withParameter("filepath", "logfs/10")
		.withParameter("datasize", CACHE_SIZE)
		.andReturnValue(CACHE_SIZE);
	expect_no_reclaim("logfs/10", &dir_size5, &file_size);
	mock().expectOneCall("fake_append")
		.withParameter("filepath", "logfs/10")
		.withParameter("datasize", CACHE_SIZE)
		.andReturnValue(CACHE_SIZE);

	logfs_write(logfs, 10, buf, sizeof(buf));
}
TEST(logfs, write_ShouldReturnEFBIG_WhenSingleLogIsLargerThanMaxSize) {
	char buf[MAX_SIZE + 1];
	LONGS_EQUAL(-EFBIG, logfs_write(logfs, 1, buf, sizeof(buf)));
}
TEST(logfs, ShouldUpdateFileSize_WhenAppending) {
}
