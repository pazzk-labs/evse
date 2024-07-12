#include "FreeRTOS.h"
#include "task.h"
#include "libmcu/metrics.h"
#include "libmcu/assert.h"
#include "logger.h"

#if (configCHECK_FOR_STACK_OVERFLOW > 0)
extern void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);
LIBMCU_NO_INSTRUMENT
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
	(void)xTask;
	metrics_increase(StackOverflowCount);
	metrics_set(StackOverflowTask, (uintptr_t)pcTaskName);
	error("Stack overflow! %s", pcTaskName);
	assert(0);
}
#endif
