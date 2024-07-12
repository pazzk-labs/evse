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

#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

typedef void (*downloader_data_cb_t)(const void *data, size_t datasize,
		void *ctx);

struct downloader;

struct downloader_param {
	const char *url;
	uint32_t timeout_ms;
	size_t buffer_size;
};

struct downloader_api {
	int (*prepare)(struct downloader *self,
			const struct downloader_param *param,
			downloader_data_cb_t data_cb,
			void *cb_ctx);
	int (*process)(struct downloader *self);
	int (*cancel)(struct downloader *self);
	void (*terminate)(struct downloader *self);
};

/**
 * @brief Prepares the downloader with the given parameters.
 *
 * This function sets up the downloader instance with the specified parameters,
 * callback function, and callback context. It prepares the downloader for
 * subsequent download operations.
 *
 * @param[in] self Pointer to the downloader instance to be prepared.
 * @param[in] param Pointer to the downloader parameters structure.
 * @param[in] data_cb Callback function to be called when data is received.
 * @param[in] cb_ctx Context to be passed to the callback function.
 *
 * @return int Status code indicating the result of the preparation.
 *         0 on success,
 *         negative value on error or failure.
 */
static inline int downloader_prepare(struct downloader *self,
		const struct downloader_param *param,
		downloader_data_cb_t data_cb,
		void *cb_ctx)
{
	return ((const struct downloader_api *)self)->prepare(self,
			param, data_cb, cb_ctx);
}

/**
 * @brief Processes the downloader instance.
 *
 * This function performs the necessary operations to process the downloader
 * instance. It handles the downloading tasks and manages the state of the
 * downloader.
 *
 * @param[in] self Pointer to the downloader instance to be processed.
 *
 * @return int Status code indicating the result of the processing.
 *         0 on successful completion of the download,
 *         -EINPROGRESS if the download is still in progress,
 *         other negative values on error or failure.
 */
static inline int downloader_process(struct downloader *self)
{
	return ((const struct downloader_api *)self)->process(self);
}

/**
 * @brief Cancels the ongoing download operation.
 *
 * This function cancels the current download operation for the given downloader
 * instance. It stops any ongoing download tasks and cleans up any resources
 * associated with the download.
 *
 * @param[in] self Pointer to the downloader instance for which the download is
 *            to be canceled.
 *
 * @return int Status code indicating the result of the cancellation.
 *         0 on successful cancellation,
 *         negative value on error or failure.
 */
static inline int downloader_cancel(struct downloader *self)
{
	return ((const struct downloader_api *)self)->cancel(self);
}

/**
 * @brief Deletes the downloader instance.
 *
 * This function cleans up and deletes the downloader instance, releasing any
 * resources that were allocated during its lifetime. It prepares the downloader
 * for destruction.
 *
 * @param[in] self Pointer to the downloader instance to be deleted.
 */
static inline void downloader_delete(struct downloader *self)
{
	((const struct downloader_api *)self)->terminate(self);
}

#if defined(__cplusplus)
}
#endif

#endif /* DOWNLOADER_H */
