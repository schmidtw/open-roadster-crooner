#ifndef TASK_INTERNAL_H
#define TASK_INTERNAL_H

#include "task.h"
#include "portable.h"
#include "list.h"

/*
 * Used internally only.
 */
typedef struct xTIME_OUT
{
    portBASE_TYPE xOverflowCount;
    portTickType  xTimeOnEntering;
} xTimeOutType;

/*
 * THIS FUNCTION MUST NOT BE USED FROM APPLICATION CODE.  IT IS ONLY
 * INTENDED FOR USE WHEN IMPLEMENTING A PORT OF THE SCHEDULER AND IS
 * AN INTERFACE WHICH IS FOR THE EXCLUSIVE USE OF THE SCHEDULER.
 *
 * Called from the real time kernel tick (either preemptive or cooperative),
 * this increments the tick count and checks if any tasks that are blocked
 * for a finite period required removing from a blocked list and placing on
 * a ready list.
 */
inline void vTaskIncrementTick( void );

/*
 * THIS FUNCTION MUST NOT BE USED FROM APPLICATION CODE.  IT IS AN
 * INTERFACE WHICH IS FOR THE EXCLUSIVE USE OF THE SCHEDULER.
 *
 * THIS FUNCTION MUST BE CALLED WITH INTERRUPTS DISABLED.
 *
 * Removes the calling task from the ready list and places it both
 * on the list of tasks waiting for a particular event, and the
 * list of delayed tasks.  The task will be removed from both lists
 * and replaced on the ready list should either the event occur (and
 * there be no higher priority tasks waiting on the same event) or
 * the delay period expires.
 *
 * @param pxEventList The list containing tasks that are blocked waiting
 * for the event to occur.
 *
 * @param xTicksToWait The maximum amount of time that the task should wait
 * for the event to occur.  This is specified in kernel ticks,the constant
 * portTICK_RATE_MS can be used to convert kernel ticks into a real time
 * period.
 */
void vTaskPlaceOnEventList( const xList * const pxEventList, portTickType xTicksToWait );

/*
 * THIS FUNCTION MUST NOT BE USED FROM APPLICATION CODE.  IT IS AN
 * INTERFACE WHICH IS FOR THE EXCLUSIVE USE OF THE SCHEDULER.
 *
 * THIS FUNCTION MUST BE CALLED WITH INTERRUPTS DISABLED.
 *
 * Removes a task from both the specified event list and the list of blocked
 * tasks, and places it on a ready queue.
 *
 * xTaskRemoveFromEventList () will be called if either an event occurs to
 * unblock a task, or the block timeout period expires.
 *
 * @return pdTRUE if the task being removed has a higher priority than the task
 * making the call, otherwise pdFALSE.
 */
signed portBASE_TYPE xTaskRemoveFromEventList( const xList * const pxEventList );

/*
 * THIS FUNCTION MUST NOT BE USED FROM APPLICATION CODE.  IT IS AN
 * INTERFACE WHICH IS FOR THE EXCLUSIVE USE OF THE SCHEDULER.
 *
 * INCLUDE_vTaskCleanUpResources and INCLUDE_vTaskSuspend must be defined as 1
 * for this function to be available.
 * See the configuration section for more information.
 *
 * Empties the ready and delayed queues of task control blocks, freeing the
 * memory allocated for the task control block and task stacks as it goes.
 */
void vTaskCleanUpResources( void );

/*
 * THIS FUNCTION MUST NOT BE USED FROM APPLICATION CODE.  IT IS ONLY
 * INTENDED FOR USE WHEN IMPLEMENTING A PORT OF THE SCHEDULER AND IS
 * AN INTERFACE WHICH IS FOR THE EXCLUSIVE USE OF THE SCHEDULER.
 *
 * Sets the pointer to the current TCB to the TCB of the highest priority task
 * that is ready to run.
 */
inline void vTaskSwitchContext( void );

/*-----------------------------------------------------------
 * SCHEDULER INTERNALS AVAILABLE FOR PORTING PURPOSES
 *----------------------------------------------------------*/

/*  Definitions returned by xTaskGetSchedulerState(). */
#define taskSCHEDULER_NOT_STARTED   0
#define taskSCHEDULER_RUNNING       1
#define taskSCHEDULER_SUSPENDED     2

/*
 * Capture the current time status for future reference.
 */
void vTaskSetTimeOutState( xTimeOutType * const pxTimeOut );

/*
 * Compare the time status now with that previously captured to see if the
 * timeout has expired.
 */
portBASE_TYPE xTaskCheckForTimeOut( xTimeOutType * const pxTimeOut, portTickType * const pxTicksToWait );

/*
 * Shortcut used by the queue implementation to prevent unnecessary call to
 * taskYIELD();
 */
void vTaskMissedYield( void );

/*
 * Returns the scheduler state as taskSCHEDULER_RUNNING,
 * taskSCHEDULER_NOT_STARTED or taskSCHEDULER_SUSPENDED.
 */
portBASE_TYPE xTaskGetSchedulerState( void );

/*
 * Raises the priority of the mutex holder to that of the calling task should
 * the mutex holder have a priority less than the calling task.
 */
void vTaskPriorityInherit( xTaskHandle * const pxMutexHolder );

/*
 * Set the priority of a task back to its proper priority in the case that it
 * inherited a higher priority while it was holding a semaphore.
 */
void vTaskPriorityDisinherit( xTaskHandle * const pxMutexHolder );

#endif
