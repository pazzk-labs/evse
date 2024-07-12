#include "libmcu/ratelim.h"

void ratelim_init(struct ratelim *bucket, const ratelim_unit_t unit,
		const uint32_t cap, const uint32_t leak_rate) {
	(void)bucket;
	(void)unit;
	(void)cap;
	(void)leak_rate;
}

bool ratelim_request(struct ratelim *bucket) {
	(void)bucket;
	return false;
}

bool ratelim_request_ext(struct ratelim *bucket, const uint32_t n) {
	(void)bucket;
	(void)n;
	return false;
}

bool ratelim_request_format(struct ratelim *bucket,
		ratelim_format_func_t func, const char *format, ...) {
	(void)bucket;
	(void)func;
	(void)format;
	return false;
}

bool ratelim_full(struct ratelim *bucket) {
	(void)bucket;
	return false;
}
