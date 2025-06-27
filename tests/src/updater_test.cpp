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

#include "updater.h"
#include "downloader.h"
#include "libmcu/dfu.h"
#include <time.h>

static downloader_data_cb_t do_data_cb;
static void *do_data_cb_ctx;

static time_t now;

static void runner(void *ctx) {
	mock().actualCall("runner").withParameter("ctx", ctx);
}

static void on_event(updater_event_t event, void *ctx) {
	mock().actualCall("on_event")
		.withParameter("event", event);
}

static int my_prepare(struct downloader *self,
		const struct downloader_param *param,
		downloader_data_cb_t data_cb,
		void *cb_ctx) {
	do_data_cb = data_cb;
	do_data_cb_ctx = cb_ctx;
	return mock().actualCall("downloader_prepare").returnIntValue();
}
static int my_process(struct downloader *self) {
	return mock().actualCall("downloader_process").returnIntValue();
}
static int my_cancel(struct downloader *self) {
	return mock().actualCall("downloader_cancel").returnIntValue();
}
static void my_terminate(struct downloader *self) {
	mock().actualCall("downloader_delete");
}

static struct downloader_api my_downloader = {
	.prepare = my_prepare,
	.process = my_process,
	.cancel = my_cancel,
	.terminate = my_terminate,
};

time_t time(time_t *t) {
	if (t) {
		*t = now;
	}
	return now;
}

TEST_GROUP(updater) {
	void setup(void) {
		now = 0;
		updater_init();
	}
	void teardown(void) {
		updater_deinit();

		mock().checkExpectations();
		mock().clear();
	}

	void set_time(time_t t) {
		now = t;
	}

	void go_start_downloading(void) {
		struct downloader *downloader =
			(struct downloader *)&my_downloader;
		struct updater_param param = {
			.retrieve_date = time(NULL),
		};
		updater_request(&param, downloader);
		mock().expectOneCall("dfu_new")
			.ignoreOtherParameters().andReturnValue((void *)0x5678);
		mock().expectOneCall("downloader_prepare").andReturnValue(0);
		LONGS_EQUAL(-EAGAIN, updater_process());
	}
};

TEST(updater, ShouldReturnEINVAL_WhenNullRunnerGiven) {
	LONGS_EQUAL(-EINVAL, updater_set_runner(NULL, NULL));
}
TEST(updater, ShouldReturnZero_WhenRunnerGiven) {
	LONGS_EQUAL(0, updater_set_runner(runner, NULL));
}
TEST(updater, ShouldCallRunner_WhenRequestAccepted) {
	updater_set_runner(runner, NULL);

	struct downloader *downloader = (struct downloader *)0x1234;
        struct updater_param param = {
		.retrieve_date = (time_t)1736870825,
	};
	mock().expectOneCall("runner").withParameter("ctx", (void *)NULL);
	updater_request(&param, downloader);
}
TEST(updater, ShouldCallRunnerWithContext_WhenContextGiven) {
	int ctx;
	updater_set_runner(runner, &ctx);

	struct downloader *downloader = (struct downloader *)0x1234;
        struct updater_param param = {
		.retrieve_date = time(NULL) + 1,
	};
	mock().expectOneCall("runner").withParameter("ctx", (void *)&ctx);
	updater_request(&param, downloader);
}
TEST(updater, ShouldReturnZero_WhenNoRequest) {
	LONGS_EQUAL(0, updater_process());
}
TEST(updater, ShouldReturnEAGAIN_WhenPending) {
	struct downloader *downloader = (struct downloader *)0x1234;
        struct updater_param param = {
		.retrieve_date = time(NULL) + 1,
	};
	updater_request(&param, downloader);
	LONGS_EQUAL(-EAGAIN, updater_process());
}
TEST(updater, ShouldReturnZero_WhenDfuNewFailed) {
	struct downloader *downloader = (struct downloader *)0x1234;
        struct updater_param param = {
		.retrieve_date = time(NULL),
	};
	updater_request(&param, downloader);
	mock().expectOneCall("dfu_new")
		.ignoreOtherParameters().andReturnValue((void *)NULL);
	LONGS_EQUAL(UPDATER_EVT_ERROR_PREPARING, updater_process());
}

TEST(updater, ShouldDispatchEvent_WhenDfuNewFailed) {
	struct downloader *downloader = (struct downloader *)&my_downloader;
	struct updater_param param = {
		.retrieve_date = time(NULL),
	};
	updater_request(&param, downloader);
	updater_register_event_callback(on_event, NULL);
	mock().expectOneCall("dfu_new")
		.ignoreOtherParameters().andReturnValue((void *)NULL);
	mock().expectOneCall("on_event")
		.withParameter("event", UPDATER_EVT_ERROR_PREPARING);
	LONGS_EQUAL(UPDATER_EVT_ERROR_PREPARING, updater_process());
}

TEST(updater, ShouldReturnZero_WhenDownloaderPrepareFailed) {
	struct downloader *downloader = (struct downloader *)&my_downloader;
        struct updater_param param = {
		.retrieve_date = time(NULL),
	};
	updater_request(&param, downloader);
	mock().expectOneCall("dfu_new")
		.ignoreOtherParameters().andReturnValue((void *)0x5678);
	mock().expectOneCall("downloader_prepare").andReturnValue(-ENOMEM);
	mock().expectOneCall("dfu_delete");
	LONGS_EQUAL(UPDATER_EVT_ERROR_PREPARING, updater_process());
}
TEST(updater, ShouldReturnError_WhenStartDownloading) {
	struct downloader *downloader = (struct downloader *)&my_downloader;
	struct updater_param param = {
		.retrieve_date = time(NULL),
	};
	updater_request(&param, downloader);
	mock().expectOneCall("dfu_new")
		.ignoreOtherParameters().andReturnValue((void *)0x5678);
	mock().expectOneCall("downloader_prepare").andReturnValue(0);
	LONGS_EQUAL(-EAGAIN, updater_process());
}
TEST(updater, ShouldReturnError_WhenDownloading) {
	go_start_downloading();
	mock().expectOneCall("downloader_process").andReturnValue(-EINPROGRESS);
	LONGS_EQUAL(-EAGAIN, updater_process());
}
TEST(updater, ShouldReturnError_WhenErrorDownloading) {
	go_start_downloading();
	mock().expectOneCall("downloader_process").andReturnValue(-EIO);
	mock().expectOneCall("dfu_finish").andReturnValue(DFU_ERROR_NONE);
	mock().expectOneCall("dfu_abort").andReturnValue(DFU_ERROR_NONE);
	mock().expectOneCall("dfu_delete");
	mock().expectOneCall("downloader_delete");
	LONGS_EQUAL(UPDATER_EVT_ERROR_DOWNLOADING, updater_process());
}
TEST(updater, ShouldReturnError_WhenErrorFinishing) {
	go_start_downloading();
	mock().expectOneCall("downloader_process").andReturnValue(0);
	mock().expectOneCall("dfu_finish").andReturnValue(DFU_ERROR_IO);
	mock().expectOneCall("dfu_abort").andReturnValue(DFU_ERROR_NONE);
	mock().expectOneCall("dfu_delete");
	mock().expectOneCall("downloader_delete");
	LONGS_EQUAL(UPDATER_EVT_ERROR_DOWNLOADING, updater_process());
}
TEST(updater, ShouldReturnDone_WhenSuccess) {
	go_start_downloading();
	mock().expectOneCall("downloader_process").andReturnValue(0);
	mock().expectOneCall("dfu_finish").andReturnValue(DFU_ERROR_NONE);
	mock().expectOneCall("dfu_commit").andReturnValue(DFU_ERROR_NONE);
	mock().expectOneCall("dfu_delete");
	mock().expectOneCall("downloader_delete");
	LONGS_EQUAL(UPDATER_EVT_INSTALLED, updater_process());
}
TEST(updater, ShouldReturnError_WhenInvalidHeaderGiven) {
	const uint8_t data[] = { 0x01, 0x02, 0x03, 0x04 };
	go_start_downloading();

	mock().expectOneCall("dfu_is_valid_header").ignoreOtherParameters()
		.andReturnValue(false);
	do_data_cb(data, sizeof(data), do_data_cb_ctx);

	mock().expectOneCall("downloader_process").andReturnValue(-EINPROGRESS);
	mock().expectOneCall("dfu_finish").andReturnValue(DFU_ERROR_NONE);
	mock().expectOneCall("dfu_abort").andReturnValue(DFU_ERROR_NONE);
	mock().expectOneCall("dfu_delete");
	mock().expectOneCall("downloader_delete");
	LONGS_EQUAL(UPDATER_EVT_ERROR_DOWNLOADING, updater_process());
}
TEST(updater, ShouldReturnError_WhenDfuPrepareFailed) {
	const uint8_t data[] = { 0x01, 0x02, 0x03, 0x04 };
	go_start_downloading();

	mock().expectOneCall("dfu_is_valid_header").ignoreOtherParameters()
		.andReturnValue(true);
	mock().expectOneCall("dfu_prepare").ignoreOtherParameters()
		.andReturnValue(DFU_ERROR_IO);
	do_data_cb(data, sizeof(data), do_data_cb_ctx);

	mock().expectOneCall("downloader_process").andReturnValue(-EINPROGRESS);
	mock().expectOneCall("dfu_finish").andReturnValue(DFU_ERROR_NONE);
	mock().expectOneCall("dfu_abort").andReturnValue(DFU_ERROR_NONE);
	mock().expectOneCall("dfu_delete");
	mock().expectOneCall("downloader_delete");
	LONGS_EQUAL(UPDATER_EVT_ERROR_DOWNLOADING, updater_process());
}
TEST(updater, ShouldWriteData_WhenValidHeaderGiven) {
	const uint8_t data[] = { 0x01, 0x02, 0x03, 0x04 };
	go_start_downloading();

	mock().expectOneCall("dfu_is_valid_header").ignoreOtherParameters()
		.andReturnValue(true);
	mock().expectOneCall("dfu_prepare").ignoreOtherParameters()
		.andReturnValue(DFU_ERROR_NONE);
	mock().expectOneCall("dfu_write").ignoreOtherParameters()
		.andReturnValue(0);
	do_data_cb(data, sizeof(data), do_data_cb_ctx);

	mock().expectOneCall("downloader_process").andReturnValue(-EINPROGRESS);
	LONGS_EQUAL(-EAGAIN, updater_process());
}
TEST(updater, ShouldReturnError_WhenWriteDataFailed) {
	const uint8_t data[] = { 0x01, 0x02, 0x03, 0x04 };
	go_start_downloading();

	mock().expectOneCall("dfu_is_valid_header").ignoreOtherParameters()
		.andReturnValue(true);
	mock().expectOneCall("dfu_prepare").ignoreOtherParameters()
		.andReturnValue(DFU_ERROR_NONE);
	mock().expectOneCall("dfu_write").ignoreOtherParameters()
		.andReturnValue(-EIO);
	do_data_cb(data, sizeof(data), do_data_cb_ctx);

	mock().expectOneCall("downloader_process").andReturnValue(-EINPROGRESS);
	mock().expectOneCall("dfu_finish").andReturnValue(DFU_ERROR_NONE);
	mock().expectOneCall("dfu_abort").andReturnValue(DFU_ERROR_NONE);
	mock().expectOneCall("dfu_delete");
	mock().expectOneCall("downloader_delete");
	LONGS_EQUAL(UPDATER_EVT_ERROR_DOWNLOADING, updater_process());
}

TEST(updater, ShouldReturnFalse_WhenNoRequest) {
	LONGS_EQUAL(false, updater_busy());
}
TEST(updater, ShouldReturnTrue_WhenInprogress) {
	go_start_downloading();
	LONGS_EQUAL(true, updater_busy());
}

TEST(updater, ShouldDispatchEveryEvents_WhenDone) {
	go_start_downloading();
	updater_register_event_callback(on_event, NULL);
	mock().expectOneCall("on_event")
		.withParameter("event", UPDATER_EVT_DOWNLOADED);
	mock().expectOneCall("on_event")
		.withParameter("event", UPDATER_EVT_START_INSTALLING);
	mock().expectOneCall("on_event")
		.withParameter("event", UPDATER_EVT_INSTALLED);
	mock().expectOneCall("on_event")
		.withParameter("event", UPDATER_EVT_REBOOT_REQUIRED);

	mock().expectOneCall("downloader_process").andReturnValue(0);
	mock().expectOneCall("dfu_finish").andReturnValue(DFU_ERROR_NONE);
	mock().expectOneCall("dfu_commit").andReturnValue(DFU_ERROR_NONE);
	mock().expectOneCall("dfu_delete");
	mock().expectOneCall("downloader_delete");
	LONGS_EQUAL(UPDATER_EVT_INSTALLED, updater_process());
}

TEST(updater, ShouldReturnError_WhenTimedOut) {
	go_start_downloading();

	set_time(UPDATER_TIMEOUT_SEC);
	mock().expectOneCall("downloader_process").andReturnValue(-EINPROGRESS);
	mock().expectOneCall("dfu_finish").andReturnValue(DFU_ERROR_NONE);
	mock().expectOneCall("dfu_abort").andReturnValue(DFU_ERROR_NONE);
	mock().expectOneCall("dfu_delete");
	mock().expectOneCall("downloader_delete");

	LONGS_EQUAL(UPDATER_EVT_ERROR_TIMEOUT, updater_process());
}
