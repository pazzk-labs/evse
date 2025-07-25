#include "CppUTestExt/MockSupport.h"
#include "uptime.h"

time_t uptime_get(void) {
	return (time_t)mock().actualCall(__func__).returnLongLongIntValue();
}

void uptime_init(void) {
	mock().actualCall(__func__);
}
