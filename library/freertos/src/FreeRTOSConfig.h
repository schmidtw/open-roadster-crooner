#ifndef _FREERTOS_CONFIG_H_
#define _FREERTOS_CONFIG_H_

#include <bsp/boards/boards.h>

#ifndef FOSC0
#error "FOSC0 must be defined."
#endif

/* System Configuration */
#define configUSE_PREEMPTION            1
#define configUSE_IDLE_HOOK             0
#define configUSE_TICK_HOOK             0
#define configTICK_RATE_HZ              1000
#define configMAX_PRIORITIES            4
#define configMINIMAL_STACK_SIZE        192
#define configMAX_TASK_NAME_LEN         8
#define configUSE_16_BIT_TICKS          0
#define configIDLE_SHOULD_YIELD         1
#define configUSE_MUTEXES               1
#define configUSE_RECURSIVE_MUTEXES     0
#define configUSE_COUNTING_SEMAPHORES   0
#define configUSE_ALTERNATIVE_API       0
#define configCHECK_FOR_STACK_OVERFLOW  0
#define configUSE_CO_ROUTINES           0
#define configMAX_CO_ROUTINE_PRIORITIES 0
#define configKERNEL_INTERRUPT_PRIORITY 0
#define configUSE_TRACE_FACILITY        0

/* Code inclusion optimizations */
#define INCLUDE_vTaskPrioritySet            1
#define INCLUDE_uxTaskPriorityGet           0
#define INCLUDE_vTaskDelete                 0
#define INCLUDE_vTaskCleanUpResources       0
#define INCLUDE_vTaskSuspend                1
#define INCLUDE_vTaskDelayUntil             0
#define INCLUDE_vTaskDelay                  1
#define INCLUDE_xTaskGetCurrentTaskHandle   1
#define INCLUDE_xTaskGetSchedulerState		0
#define INCLUDE_xTaskResumeFromISR          1
#endif
