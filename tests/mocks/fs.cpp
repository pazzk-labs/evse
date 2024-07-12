#include "CppUTestExt/MockSupport.h"
#include "fs/fs.h"

struct fs {
	struct fs_api api;
};

static int do_mount(struct fs *self) {
	return mock().actualCall(__func__).returnIntValue();
}

static int do_unmount(struct fs *self) {
	return mock().actualCall(__func__).returnIntValue();
}

static int do_read(struct fs *self, const char *filepath,
		const size_t offset, void *buf, const size_t bufsize) {
	return mock().actualCall(__func__)
		.withParameter("filepath", filepath)
		.withParameter("offset", offset)
		.withOutputParameter("buf", buf)
		.returnIntValue();
}

static int do_write(struct fs *self, const char *filepath,
		const size_t offset, const void *data, const size_t datasize) {
	return mock().actualCall(__func__)
		.withParameter("filepath", filepath)
		.withParameter("offset", offset)
		.withParameter("data", data)
		.withParameter("datasize", datasize)
		.returnIntValue();
}

static int do_append(struct fs *self, const char *filepath,
		const void *data, const size_t datasize) {
	return mock().actualCall(__func__)
		.withParameter("filepath", filepath)
		.withParameter("data", data)
		.withParameter("datasize", datasize)
		.returnIntValue();
}

static int do_erase(struct fs *self, const char *filepath) {
	return mock().actualCall(__func__)
		.withParameter("filepath", filepath)
		.returnIntValue();
}

static int do_size(struct fs *self, const char *filepath, size_t *size) {
	return mock().actualCall(__func__)
		.withParameter("filepath", filepath)
		.withOutputParameter("size", size)
		.returnIntValue();
}

static int do_dir(struct fs *self, const char *path,
		fs_dir_cb_t cb, void *cb_ctx) {
	return mock().actualCall(__func__).returnIntValue();
}

static int do_usage(struct fs *self, size_t *used, size_t *total) {
	return mock().actualCall(__func__)
		.withOutputParameter("used", used)
		.withOutputParameter("total", total)
		.returnIntValue();
}

struct fs *fs_create(struct flash *flash) {
	struct fs *fs = new struct fs;
	fs->api = (struct fs_api) {
		.mount = do_mount,
		.unmount = do_unmount,
		.read = do_read,
		.write = do_write,
		.append = do_append,
		.erase = do_erase,
		.size = do_size,
		.dir = do_dir,
		.usage = do_usage,
	};
	return fs;
}

void fs_destroy(struct fs *fs) {
	delete fs;
}
