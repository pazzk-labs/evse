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

#ifndef LOGFS_H
#define LOGFS_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "fs/fs.h"
#include <time.h>

#if !defined(LOGFS_MIN_CACHE_SIZE)
#define LOGFS_MIN_CACHE_SIZE		4096
#endif

struct logfs;

/**
 * @brief Callback function type for iterating over log files.
 *
 * This callback function is called for each log file in the log filesystem
 * when using the logfs_dir function.
 *
 * @param[in] fs Pointer to the log filesystem structure.
 * @param[in] timestamp The timestamp used as the log file name.
 *
 * @param[in] ctx User-defined context passed to the callback function.
 */
typedef void (*logfs_dir_cb_t)(struct logfs *fs,
		const time_t timestamp, void *ctx);

/**
 * @brief Creates a log filesystem structure.
 *
 * This function initializes and returns a pointer to a log filesystem
 * structure. The log filesystem is used to manage log files with a specified
 * maximum size and maximum number of logs.
 *
 * @note Logs are sorted in ascending order by filename.
 *
 * @note If the total size of the logs exceeds max_size, logs are deleted in
 * sorted order until the total size is less than max_size. For example, if
 * there are files "1" and "2", the file "1" will be deleted first.
 *
 * @note If the number of stored logs exceeds max_logs, logs are deleted in
 * sorted order until the the number of logs is less than max_logs.
 *
 * @note If max_size or max_logs is 0, it is considered unlimited.
 *
 * @param[in] fs Pointer to the underlying filesystem structure.
 * @param[in] base_path The base path where log files will be stored.
 * @param[in] max_size The maximum size (in bytes) for the log filesystem.
 * @param[in] max_logs The maximum number of log files to be maintained.
 * @param[in] cache_size The size of the cache (in bytes) for buffering log
 *            data.
 *
 * @return Pointer to the created log filesystem structure.
 */
struct logfs *logfs_create(struct fs *fs, const char *base_path,
		const size_t max_size, const size_t max_logs,
		size_t cache_size);

/**
 * @brief Destroys the log filesystem structure.
 *
 * This function releases all resources associated with the log filesystem
 * structure.
 *
 * @param[in] self Pointer to the log filesystem structure to be destroyed.
 */
void logfs_destroy(struct logfs *self);

/**
 * @brief Writes data to a log file.
 *
 * This function writes the specified data to the given log file within the log
 * filesystem.
 *
 * @note If the log file does not exist, it will be created.
 *
 * @note If the log file exists, the data will be appended to the end of the
 *       file.
 *
 * @param[in] self Pointer to the log filesystem structure.
 * @param[in] timestamp The timestamp used as the log file name.
 * @param[in] log Pointer to the data to be written.
 * @param[in] logsize The size of the data to be written in bytes.
 *
 * @return A positive value indicating the number of bytes actually written on
 *         success, or a negative error code on failure.
 */
int logfs_write(struct logfs *self,
		const time_t timestamp, const void *log, const size_t logsize);

/**
 * @brief Reads data from a log file.
 *
 * This function reads data from the specified log file within the log
 * filesystem.
 *
 * @param[in] self Pointer to the log filesystem structure.
 * @param[in] timestamp The timestamp used as the log file name.
 * @param[in] offset The offset within the file to start reading from.
 * @param[out] buf Pointer to the buffer where the read data will be stored.
 * @param[in] bufsize The size of the buffer in bytes.
 *
 * @return The number of bytes read on success, or a negative error code on
 *         failure.
 */
int logfs_read(struct logfs *self, const time_t timestamp,
		const size_t offset, void *buf, const size_t bufsize);

/**
 * @brief Gets the size of a log file or the total size of all log files.
 *
 * This function returns the size of the specified log file within the log
 * filesystem.
 *
 * @param[in] self Pointer to the log filesystem structure.
 * @param[in] timestamp The timestamp used as the log file name. If 0, the
 *            total size of all log files is returned.
 *
 * @return The size of the specified log file in bytes, or the total size of
 *         all log files if timestamp is 0. If the return value is 0, it may
 *         indicate an error or that the file does not exist.
 */
size_t logfs_size(struct logfs *self, const time_t timestamp);

/**
 * @brief Get the count of log entries in the log file system.
 *
 * @param[in] self Pointer to the log file system instance.
 *
 * @return The number of log entries.
 */
size_t logfs_count(struct logfs *self);

/**
 * @brief Lists log files in the log filesystem up to a specified maximum
 * number.
 *
 * This function iterates over log files in the log filesystem and calls the
 * provided callback function for each file, up to the specified maximum number
 * of files.
 *
 * @note If max_files is 0, all log files will be listed.
 *
 * @param[in] self Pointer to the log filesystem structure.
 * @param[in] cb Callback function to be called for each log file.
 * @param[in] cb_ctx User-defined context to be passed to the callback function.
 * @param[in] max_files The maximum number of log files to list. If 0, all log
 *            files will be listed.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int logfs_dir(struct logfs *self, logfs_dir_cb_t cb, void *cb_ctx,
		const size_t max_files);

/**
 * @brief Deletes a log file.
 *
 * This function deletes the specified log file from the log filesystem.
 *
 * @param[in] self Pointer to the log filesystem structure.
 * @param[in] timestamp The timestamp used as the log file name.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int logfs_delete(struct logfs *self, const time_t timestamp);

/**
 * @brief Clears all log files.
 *
 * This function deletes all log files from the log filesystem.
 *
 * @param[in] self Pointer to the log filesystem structure.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int logfs_clear(struct logfs *self);

/**
 * @brief Flushes the log filesystem.
 *
 * This function flushes any buffered data to the log files within the log
 * filesystem. It ensures that all pending writes are completed and the data is
 * written to the disk.
 *
 * @param[in] self Pointer to the log filesystem structure.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int logfs_flush(struct logfs *self);

#if defined(__cplusplus)
}
#endif

#endif /* LOGFS_H */
