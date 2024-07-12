#include "libmcu/logging.h"
#include <stdio.h>
#include <stdarg.h>

size_t logging_write(logging_t type, const struct logging_context *ctx, ...)
{
	int len = printf("[%s] <%p,%p> ",
			type == 0? "DEBUG" :
			type == 1? "INFO" :
			type == 2? "WARN" :
			type == 3? "ERROR" :
			type == 4? "NONE" : "UNKNOWN",
			ctx->pc, ctx->lr);
	va_list ap;
	va_start(ap, ctx);
	const char *fmt = va_arg(ap, char *);
	if (fmt) {
		len += vfprintf(stdout, fmt, ap);
		len += printf("\n");
	}
	return (size_t)len;
}
