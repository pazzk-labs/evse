/*
 * This file is part of the Pazzk project <https://pazzk.net/>.
 * Copyright (c) 2024 Pazzk <team@pazzk.net>.
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

#include "updater.h"

#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "libmcu/dfu.h"
#include "libmcu/ringbuf.h"
#include "libmcu/metrics.h"
#include "downloader.h"
#include "logger.h"

#if !defined(MIN)
#define MIN(a, b)		((a) > (b) ? (b) : (a))
#endif

struct updater {
	struct updater_param param;
	struct updater_param *current;

	pthread_mutex_t lock;

	struct dfu *dfu;
	struct downloader *downloader;
	struct ringbuf *ringbuf;

	updater_event_callback_t event_cb;
	void *event_cb_ctx;

	updater_runner_t runner;
	void *runner_ctx;

	struct {
		time_t requested;
		time_t started;
	} timestamp;

	size_t bytes_total;
	size_t bytes_written;
	int error;
};

static struct updater updater;

static void clear_request(struct updater *self)
{
	self->current = NULL;
	self->dfu = NULL;
	ringbuf_consume(self->ringbuf, ringbuf_length(self->ringbuf));
}

static void set_request(struct updater *self, const struct updater_param *param,
		struct downloader *downloader)
{
	memcpy(&self->param, param, sizeof(*param));
	self->downloader = downloader;
	self->timestamp.requested = time(NULL);
	self->error = 0;
	self->bytes_written = 0;
	self->current = &self->param;
}

static bool is_param_set(struct updater *self)
{
	if (self->current) {
		return true;
	}
	return false;
}

static bool is_pending(struct updater *self)
{
	if (is_param_set(self) && !self->dfu) {
		return true;
	}
	return false;
}

static bool is_updating(struct updater *self)
{
	if (is_param_set(self) && self->dfu) {
		return true;
	}
	return false;
}

static bool has_date_arrived(struct updater *self, const time_t *now)
{
	if (self->current->retrieve_date <= *now) {
		return true;
	}
	return false;
}

static bool is_request_inprogress(updater_event_t event)
{
	return event == UPDATER_EVT_START_DOWNLOADING ||
		event == UPDATER_EVT_DOWNLOADING ||
		event == UPDATER_EVT_DOWNLOADED ||
		event == UPDATER_EVT_START_INSTALLING;
}

static bool is_timedout(const time_t *started, const time_t *now,
		const int timeout_sec)
{
	if (*now - *started >= timeout_sec) {
		return true;
	}
	return false;
}

static int prepare_write(struct updater *self)
{
	if (self->bytes_written == 0 && self->error == 0) { /* first block */
		const struct dfu_image_header *hdr =
			(const struct dfu_image_header *)
			ringbuf_peek_pointer(self->ringbuf, 0, NULL);

		if (!dfu_is_valid_header(hdr) || dfu_prepare(self->dfu, hdr)
				!= DFU_ERROR_NONE) {
			error("Invalid DFU header.");
			return -EPROTO;
		}

		self->bytes_total = hdr->datasize;
		ringbuf_consume(self->ringbuf, sizeof(*hdr));

		info("DFU: type=%u, datasize=%u", hdr->type, hdr->datasize);
	}

	return 0;
}

static int write_data_block(struct updater *self)
{
	size_t len = 0;
	const void *data = ringbuf_peek_pointer(self->ringbuf, 0, &len);
	/* NOTE: the DFU buffer size is also allocated as UPDATER_BUFFER_SIZE,
	 * so data larger than UPDATER_BUFFER_SIZE cannot be processed. */
	len = MIN(len, UPDATER_BUFFER_SIZE);

	if (data && len) {
		if (dfu_write(self->dfu, 0, data, len) < 0) {
			return -EIO;
		}
		ringbuf_consume(self->ringbuf, len);

		self->bytes_written += len;
	}

	return 0;
}

static int write_to_flash(struct updater *self)
{
	int err = self->error;

	if (err || ringbuf_length(self->ringbuf) == 0 ||
			(err = prepare_write(self)) != 0) {
		return err;
	}

	while (ringbuf_length(self->ringbuf) > 0) {
		if ((err = write_data_block(self)) != 0) {
			return err;
		}
	}

	return 0;
}

static void on_data_reception(const void *data, size_t datasize, void *ctx)
{
	struct updater *self = (struct updater *)ctx;

	const size_t written = ringbuf_write(self->ringbuf, data, datasize);

	debug("Received %u, %u/%u: +%u bytes", datasize,
			self->bytes_written, self->bytes_total, written);

	self->error = write_to_flash(self);
}

static updater_event_t finish_download(struct updater *self, const time_t *now,
		const int err)
{
	updater_event_t event = (err == -ETIMEDOUT)?
		UPDATER_EVT_ERROR_TIMEOUT : UPDATER_EVT_ERROR_DOWNLOADING;

	if (dfu_finish(self->dfu) == DFU_ERROR_NONE && !err && !self->error) {
		event = UPDATER_EVT_ERROR_INSTALLING;
		if (dfu_commit(self->dfu) == DFU_ERROR_NONE) {
			event = UPDATER_EVT_INSTALLED;

			const int32_t elapsed =
				(int32_t)(*now - self->timestamp.started);
			metrics_set(UpdaterCompletionTime, elapsed);
			info("Downloaded %u bytes in %u seconds",
					self->bytes_written, elapsed);
		}
	}

	if (event != UPDATER_EVT_INSTALLED) {
		dfu_abort(self->dfu);
		error("Failed to download image.");
	}

	dfu_delete(self->dfu);
	downloader_delete(self->downloader);
	clear_request(self);

	return event;
}

static updater_event_t start_download(struct updater *self, const time_t *now)
{
	updater_event_t event = UPDATER_EVT_ERROR_PREPARING;

	if ((self->dfu = dfu_new(UPDATER_BUFFER_SIZE)) == NULL) {
		error("Failed to create DFU.");
		goto out;
	}

	if (downloader_prepare(self->downloader,
			&(const struct downloader_param) {
				.url = self->param.url,
				.timeout_ms = UPDATER_TRANSFER_TIMEOUT_MS,
				.buffer_size = UPDATER_BUFFER_SIZE
			}, on_data_reception, self) != 0) {
		dfu_delete(self->dfu);
		error("Failed to initialize downloader.");
		goto out;
	}

	event = UPDATER_EVT_START_DOWNLOADING;
	self->timestamp.started = *now;
	info("Downloading image from %s", self->param.url);
out:
	if (event != UPDATER_EVT_START_DOWNLOADING) {
		clear_request(self);
	}
	return event;
}

static void dispatch_event(struct updater *self, updater_event_t event)
{
	updater_event_callback_t cb = self->event_cb;
	void *ctx = self->event_cb_ctx;

	/* UPDATER_EVT_DOWNLOADING will not be dispatched to the callback to
	 * avoid performance degradation. */
	if (event == UPDATER_EVT_NONE || !cb ||
			event == UPDATER_EVT_DOWNLOADING) {
		return;
	}

	if (event > UPDATER_EVT_DOWNLOADED) {
		(*cb)(UPDATER_EVT_DOWNLOADED, ctx);
	}
	if (event > UPDATER_EVT_START_INSTALLING) {
		(*cb)(UPDATER_EVT_START_INSTALLING, ctx);
	}

	(*cb)(event, ctx);

	if (event == UPDATER_EVT_INSTALLED) {
		(*cb)(UPDATER_EVT_REBOOT_REQUIRED, ctx);
	}
}

int updater_process(void)
{
	const time_t now = time(NULL);
	updater_event_t event = UPDATER_EVT_NONE;

	if (!is_updating(&updater)) {
		if (is_pending(&updater) && has_date_arrived(&updater, &now)) {
			pthread_mutex_lock(&updater.lock);
			event = start_download(&updater, &now);
			pthread_mutex_unlock(&updater.lock);
			metrics_increase(UpdaterStartCount);
		}
	} else {
		int err = downloader_process(updater.downloader);
		const bool downloading = err == -EINPROGRESS && !updater.error;
		const bool timedout = is_timedout(&updater.timestamp.started,
				&now, UPDATER_TIMEOUT_SEC);

		if (downloading && !timedout) {
			event = UPDATER_EVT_DOWNLOADING;
		} else {
			if (timedout) {
				err = -ETIMEDOUT;
			}

			pthread_mutex_lock(&updater.lock);
			event = finish_download(&updater, &now, err);
			pthread_mutex_unlock(&updater.lock);
		}
	}

	dispatch_event(&updater, event);

	return is_request_inprogress(event)? -EAGAIN : (int)event;
}

int updater_request(const struct updater_param *param,
		struct downloader *downloader)
{
	metrics_increase(UpdaterRequestCount);

	if (!param || !downloader) {
		return -EINVAL;
	} else if (is_updating(&updater)) {
		return -EBUSY;
	}

	if (is_pending(&updater)) {
		info("Overwriting pending request.");
	}

	pthread_mutex_lock(&updater.lock);
	set_request(&updater, param, downloader);
	pthread_mutex_unlock(&updater.lock);

	if (updater.runner) {
		(*updater.runner)(updater.runner_ctx);
	}

	info("Scheduled at %ld", (int32_t)param->retrieve_date);

	return 0;
}

bool updater_busy(void)
{
	return is_updating(&updater);
}

int updater_register_event_callback(updater_event_callback_t cb, void *cb_ctx)
{
	if (!cb) {
		return -EINVAL;
	}

	pthread_mutex_lock(&updater.lock);
	updater.event_cb = cb;
	updater.event_cb_ctx = cb_ctx;
	pthread_mutex_unlock(&updater.lock);

	return 0;
}

int updater_set_runner(updater_runner_t runner, void *ctx)
{
	if (!runner) {
		return -EINVAL;
	}

	pthread_mutex_lock(&updater.lock);
	updater.runner = runner;
	updater.runner_ctx = ctx;
	pthread_mutex_unlock(&updater.lock);

	return 0;
}

int updater_init(void)
{
	memset(&updater, 0, sizeof(updater));

	pthread_mutex_init(&updater.lock, NULL);

	if ((updater.ringbuf = ringbuf_create(UPDATER_BUFFER_SIZE)) == NULL) {
		return -ENOMEM;
	}

	return 0;
}

int updater_deinit(void)
{
	pthread_mutex_lock(&updater.lock);
	ringbuf_destroy(updater.ringbuf);
	pthread_mutex_destroy(&updater.lock);

	return 0;
}
