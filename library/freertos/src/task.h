/*
	FreeRTOS.org V5.2.0 - Copyright (C) 2003-2009 Richard Barry.

	This file is part of the FreeRTOS.org distribution.

	FreeRTOS.org is free software; you can redistribute it and/or modify it 
	under the terms of the GNU General Public License (version 2) as published
	by the Free Software Foundation and modified by the FreeRTOS exception.

	FreeRTOS.org is distributed in the hope that it will be useful,	but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
	FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for 
	more details.

	You should have received a copy of the GNU General Public License along 
	with FreeRTOS.org; if not, write to the Free Software Foundation, Inc., 59 
	Temple Place, Suite 330, Boston, MA  02111-1307  USA.

	A special exception to the GPL is included to allow you to distribute a 
	combined work that includes FreeRTOS.org without being obliged to provide
	the source code for any proprietary components.  See the licensing section
	of http://www.FreeRTOS.org for full details.


	***************************************************************************
	*                                                                         *
	* Get the FreeRTOS eBook!  See http://www.FreeRTOS.org/Documentation      *
	*                                                                         *
	* This is a concise, step by step, 'hands on' guide that describes both   *
	* general multitasking concepts and FreeRTOS specifics. It presents and   *
	* explains numerous examples that are written using the FreeRTOS API.     *
	* Full source code for all the examples is provided in an accompanying    *
	* .zip file.                                                              *
	*                                                                         *
	***************************************************************************

	1 tab == 4 spaces!

	Please ensure to read the configuration and relevant port sections of the
	online documentation.

	http://www.FreeRTOS.org - Documentation, latest information, license and
	contact details.

	http://www.SafeRTOS.com - A version that is certified for use in safety
	critical systems.

	http://www.OpenRTOS.com - Commercial support, development, porting,
	licensing and training services.
*/

/* -------------------------------------------------------------------------- */
/** \defgroup Tasks Task API
 *  @{ */
/* -------------------------------------------------------------------------- */


#ifndef TASK_H
#define TASK_H

#include "portable.h"
#include "projdefs.h"
#include "FreeRTOSConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  FreeRTOS kernel version
 */
#define tskKERNEL_VERSION_NUMBER "V5.2.0"

/**
 * Type by which tasks are referenced.  For example, a call to xTaskCreate
 * returns (via a pointer parameter) an xTaskHandle variable that can then
 * be used as a parameter to vTaskDelete to delete the task.
 */
typedef void * xTaskHandle;

/* Defines the prototype to which the application task hook function must
 * conform. */
typedef portBASE_TYPE (*pdTASK_HOOK_CODE)( void * );

/* -------------------------------------------------------------------------- */
/** \defgroup TaskLifecycle Task Lifecycle API
 *  @{ */
/* -------------------------------------------------------------------------- */


/**
 *  Create a new task and add it to the list of tasks that are ready to run.
 *
 *  @param pvTaskCode Pointer to the task entry function.  Tasks
 *  must be implemented to never return (i.e. continuous loop).
 *
 *  @param pcName A descriptive name for the task.  This is mainly used to
 *  facilitate debugging.  Max length defined by tskMAX_TASK_NAME_LEN - default
 *  is 16.
 *
 *  @param usStackDepth The size of the task stack specified as the number of
 *  variables the stack can hold - not the number of bytes.  For example, if
 *  the stack is 16 bits wide and usStackDepth is defined as 100, 200 bytes
 *  will be allocated for stack storage.
 *
 *  @param pvParameters Pointer that will be used as the parameter for the task
 *  being created.
 *
 *  @param uxPriority The priority at which the task should run.
 *
 *  @param pvCreatedTask Used to pass back a handle by which the created task
 *  can be referenced.
 *
 *  @return pdPASS if the task was successfully created and added to a ready
 *  list, otherwise an error code defined in the file errors. h
 *
 *  <b>Example usage:</b>
 *  @code
 *  // Task to be created.
 *  void vTaskCode( void * pvParameters )
 *  {
 *      for( ;; )
 *      {
 *          // Task code goes here.
 *      }
 *  }
 *
 *  // Function that creates a task.
 *  void vOtherFunction( void )
 *  {
 *      static unsigned char ucParameterToPass;
 *      xTaskHandle xHandle;
 *
 *      // Create the task, storing the handle.  Note that the passed parameter ucParameterToPass
 *      // must exist for the lifetime of the task, so in this case is declared static.  If it was just an
 *      // an automatic stack variable it might no longer exist, or at least have been corrupted, by the time
 *      // the new time attempts to access it.
 *      xTaskCreate( vTaskCode, "NAME", STACK_SIZE, &ucParameterToPass, tskIDLE_PRIORITY, &xHandle );
 *
 *      // Use the handle to delete the task.
 *      vTaskDelete( xHandle );
 *  }
 *  @endcode
 */
signed portBASE_TYPE xTaskCreate( pdTASK_CODE pvTaskCode, const signed portCHAR * const pcName, unsigned portSHORT usStackDepth, void *pvParameters, unsigned portBASE_TYPE uxPriority, xTaskHandle *pvCreatedTask );


#if (1 == INCLUDE_vTaskDelete)
/**
 *  Remove a task from the RTOS real time kernels management.  The task being
 *  deleted will be removed from all ready, blocked, suspended and event lists.
 *
 *  @note  The idle task is responsible for freeing the kernel allocated
 *  memory from tasks that have been deleted.  It is therefore important that
 *  the idle task is not starved of microcontroller processing time if your
 *  application makes any calls to vTaskDelete().  Memory allocated by the
 *  task code is not automatically freed, and should be freed before the task
 *  is deleted.
 *
 *  See the demo application file death.c for sample code that utilises
 *  vTaskDelete().
 *
 *  @param pxTask The handle of the task to be deleted.  Passing NULL will
 *  cause the calling task to be deleted.
 *
 *  <b>Example usage:</b>
 *  @code
 *  void vOtherFunction( void )
 *  {
 *      xTaskHandle xHandle;
 *
 *      // Create the task, storing the handle.
 *      xTaskCreate( vTaskCode, "NAME", STACK_SIZE, NULL, tskIDLE_PRIORITY, &xHandle );
 *
 *      // Use the handle to delete the task.
 *      vTaskDelete( xHandle );
 *  }
 *  @endcode
 */
void vTaskDelete( xTaskHandle pxTask );
#endif
/** @} */


/* -------------------------------------------------------------------------- */
/** \defgroup TaskCtrl Task Control API
 *  @{ */
/* -------------------------------------------------------------------------- */


#if (1 == INCLUDE_vTaskDelay)
/**
 *  Delay a task for a given number of ticks.  The actual time that the
 *  task remains blocked depends on the tick rate.  The constant
 *  portTICK_RATE_MS can be used to calculate real time from the tick
 *  rate - with the resolution of one tick period.
 * 
 *  vTaskDelay() specifies a time at which the task wishes to unblock relative to
 *  the time at which vTaskDelay() is called.  For example, specifying a block 
 *  period of 100 ticks will cause the task to unblock 100 ticks after 
 *  vTaskDelay() is called.  vTaskDelay() does not therefore provide a good method
 *  of controlling the frequency of a cyclical task as the path taken through the 
 *  code, as well as other task and interrupt activity, will effect the frequency 
 *  at which vTaskDelay() gets called and therefore the time at which the task 
 *  next executes.  See vTaskDelayUntil() for an alternative API function designed 
 *  to facilitate fixed frequency execution.  It does this by specifying an 
 *  absolute time (rather than a relative time) at which the calling task should 
 *  unblock.
 *
 *  @param xTicksToDelay The amount of time, in tick periods, that
 *  the calling task should block.
 *
 *  <b>Example usage:</b>
 *  @code
 *  // Wait 10 ticks before performing an action.
 *  // @note
 *  // This is for demonstration only and would be better achieved
 *  // using vTaskDelayUntil().
 *  void vTaskFunction( void * pvParameters )
 *  {
 *      portTickType xDelay, xNextTime;
 *
 *      // Calc the time at which we want to perform the action
 *      // next.
 *      xNextTime = xTaskGetTickCount() + (portTickType) 10;
 *
 *      for( ;; )
 *      {
 *          xDelay = xNextTime - xTaskGetTickCount();
 *          xNextTime += (portTickType) 10;
 *
 *          // Guard against overflow
 *          if( xDelay <= (portTickType) 10 )
 *          {
 *              vTaskDelay( xDelay );
 *          }
 *
 *          // Perform action here.
 *      }
 *  }
 *  @endcode
 */
void vTaskDelay( portTickType xTicksToDelay );
#endif


#if (1 == INCLUDE_vTaskDelayUntil)
/**
 *  Delay a task until a specified time.  This function can be used by cyclical
 *  tasks to ensure a constant execution frequency.
 *
 *  This function differs from vTaskDelay() in one important aspect:  vTaskDelay() will
 *  cause a task to block for the specified number of ticks from the time vTaskDelay() is
 *  called.  It is therefore difficult to use vTaskDelay() by itself to generate a fixed
 *  execution frequency as the time between a task starting to execute and that task
 *  calling vTaskDelay() may not be fixed [the task may take a different path though the
 *  code between calls, or may get interrupted or preempted a different number of times
 *  each time it executes].
 *
 *  Whereas vTaskDelay() specifies a wake time relative to the time at which the function
 *  is called, vTaskDelayUntil() specifies the absolute (exact) time at which it wishes to
 *  unblock.
 *
 *  The constant portTICK_RATE_MS can be used to calculate real time from the tick
 *  rate - with the resolution of one tick period.
 *
 *  @param pxPreviousWakeTime Pointer to a variable that holds the time at which the
 *  task was last unblocked.  The variable must be initialised with the current time
 *  prior to its first use (see the example below).  Following this the variable is
 *  automatically updated within vTaskDelayUntil().
 *
 *  @param xTimeIncrement The cycle time period.  The task will be unblocked at
 *  time *pxPreviousWakeTime + xTimeIncrement.  Calling vTaskDelayUntil with the
 *  same xTimeIncrement parameter value will cause the task to execute with
 *  a fixed interface period.
 *
 *  <b>Example usage:</b>
 *  @code
 *  // Perform an action every 10 ticks.
 *  void vTaskFunction( void * pvParameters )
 *  {
 *      portTickType xLastWakeTime;
 *      const portTickType xFrequency = 10;
 *
 *      // Initialise the xLastWakeTime variable with the current time.
 *      xLastWakeTime = xTaskGetTickCount();
 *      for( ;; )
 *      {
 *          // Wait for the next cycle.
 *          vTaskDelayUntil( &xLastWakeTime, xFrequency );
 *
 *          // Perform action here.
 *      }
 *  }
 *  @endcode
 */
void vTaskDelayUntil( portTickType * const pxPreviousWakeTime, portTickType xTimeIncrement );
#endif


#if (1 == INCLUDE_xTaskPriorityGet)
/**
 *  Obtain the priority of any task.
 *
 *  @param pxTask Handle of the task to be queried.  Passing a NULL
 *  handle results in the priority of the calling task being returned.
 *
 *  @return The priority of pxTask.
 *
 *  <b>Example usage:</b>
 *  @code
 *  void vAFunction( void )
 *  {
 *      xTaskHandle xHandle;
 *
 *      // Create a task, storing the handle.
 *      xTaskCreate( vTaskCode, "NAME", STACK_SIZE, NULL, tskIDLE_PRIORITY, &xHandle );
 *
 *      // ...
 *
 *      // Use the handle to obtain the priority of the created task.
 *      // It was created with tskIDLE_PRIORITY, but may have changed
 *      // it itself.
 *      if( uxTaskPriorityGet( xHandle ) != tskIDLE_PRIORITY )
 *      {
 *          // The task has changed it's priority.
 *      }
 *
 *      // ...
 *
 *      // Is our priority higher than the created task?
 *      if( uxTaskPriorityGet( xHandle ) < uxTaskPriorityGet( NULL ) )
 *      {
 *          // Our priority (obtained using NULL handle) is higher.
 *      }
 *  }
 *  @endcode
 */
unsigned portBASE_TYPE uxTaskPriorityGet( xTaskHandle pxTask );
#endif


#if (1 == INCLUDE_vTaskPrioritySet)
/**
 *  Set the priority of any task.
 *
 *  A context switch will occur before the function returns if the priority
 *  being set is higher than the currently executing task.
 *
 *  @param pxTask Handle to the task for which the priority is being set.
 *  Passing a NULL handle results in the priority of the calling task being set.
 *
 *  @param uxNewPriority The priority to which the task will be set.
 *
 *  <b>Example usage:</b>
 *  @code
 *  void vAFunction( void )
 *  {
 *      xTaskHandle xHandle;
 *
 *      // Create a task, storing the handle.
 *      xTaskCreate( vTaskCode, "NAME", STACK_SIZE, NULL, tskIDLE_PRIORITY, &xHandle );
 *
 *      // ...
 *
 *      // Use the handle to raise the priority of the created task.
 *      vTaskPrioritySet( xHandle, tskIDLE_PRIORITY + 1 );
 *
 *      // ...
 *
 *      // Use a NULL handle to raise our priority to the same value.
 *      vTaskPrioritySet( NULL, tskIDLE_PRIORITY + 1 );
 *  }
 *  @endcode
 */
void vTaskPrioritySet( xTaskHandle pxTask, unsigned portBASE_TYPE uxNewPriority );
#endif


#if (1 == INCLUDE_vTaskSuspend)
/**
 *  Suspend any task.  When suspended a task will never get any microcontroller
 *  processing time, no matter what its priority.
 *
 *  Calls to vTaskSuspend are not accumulative -
 *  i.e. calling vTaskSuspend() twice on the same task still only requires one
 *  call to vTaskResume() to ready the suspended task.
 *
 *  @param pxTaskToSuspend Handle to the task being suspended.  Passing a NULL
 *  handle will cause the calling task to be suspended.
 *
 *  <b>Example usage:</b>
 *  @code
 *  void vAFunction( void )
 *  {
 *      xTaskHandle xHandle;
 *
 *      // Create a task, storing the handle.
 *      xTaskCreate( vTaskCode, "NAME", STACK_SIZE, NULL, tskIDLE_PRIORITY, &xHandle );
 *
 *      // ...
 *
 *      // Use the handle to suspend the created task.
 *      vTaskSuspend( xHandle );
 *
 *      // ...
 *
 *      // The created task will not run during this period, unless
 *      // another task calls vTaskResume( xHandle ).
 *
 *      //...
 *
 *
 *      // Suspend ourselves.
 *      vTaskSuspend( NULL );
 *
 *      // We cannot get here unless another task calls vTaskResume
 *      // with our handle as the parameter.
 *  }
 *  @endcode
 */
void vTaskSuspend( xTaskHandle pxTaskToSuspend );
#endif


#if (1 == INCLUDE_vTaskSuspend)
/**
 *  Resumes a suspended task.
 *
 *  A task that has been suspended by one of more calls to vTaskSuspend()
 *  will be made available for running again by a single call to
 *  vTaskResume().
 *
 *  @param pxTaskToResume Handle to the task being readied.
 *
 *  <b>Example usage:</b>
 *  @code
 *  void vAFunction( void )
 *  {
 *      xTaskHandle xHandle;
 *
 *      // Create a task, storing the handle.
 *      xTaskCreate( vTaskCode, "NAME", STACK_SIZE, NULL, tskIDLE_PRIORITY, &xHandle );
 *
 *      // ...
 *
 *      // Use the handle to suspend the created task.
 *      vTaskSuspend( xHandle );
 *
 *      // ...
 *
 *      // The created task will not run during this period, unless
 *      // another task calls vTaskResume( xHandle ).
 *
 *      //...
 *
 *
 *      // Resume the suspended task ourselves.
 *      vTaskResume( xHandle );
 *
 *      // The created task will once again get microcontroller processing
 *      // time in accordance with it priority within the system.
 *  }
 *  @endcode
 */
void vTaskResume( xTaskHandle pxTaskToResume );
#endif


#if (1 == INCLUDE_xTaskResumeFromISR)
/**
 *  An implementation of vTaskResume() that can be called from within an ISR.
 *
 *  A task that has been suspended by one of more calls to vTaskSuspend()
 *  will be made available for running again by a single call to
 *  xTaskResumeFromISR().
 *
 *  @param pxTaskToResume Handle to the task being readied.
 */
portBASE_TYPE xTaskResumeFromISR( xTaskHandle pxTaskToResume );
#endif
/** @} */


/* -------------------------------------------------------------------------- */
/** \defgroup SchedulerControl Scheduler Control API
 *  @{ */
/* -------------------------------------------------------------------------- */


/**
 *  Macro for forcing a context switch.
 */
#define taskYIELD() portYIELD()


/**
 *  Macro to mark the start of a critical code region.  Preemptive context
 *  switches cannot occur when in a critical region.
 *
 *  @note This may alter the stack (depending on the portable implementation)
 *  so must be used with care!
 */
#define taskENTER_CRITICAL() portENTER_CRITICAL()


/**
 *  Macro to mark the end of a critical code region.  Preemptive context
 *  switches cannot occur when in a critical region.
 *
 *  @note This may alter the stack (depending on the portable implementation)
 *  so must be used with care!
 */
#define taskEXIT_CRITICAL() portEXIT_CRITICAL()


/**
 *  Macro to disable all maskable interrupts.
 */
#define taskDISABLE_INTERRUPTS() portDISABLE_INTERRUPTS()


/**
 *  Macro to enable microcontroller interrupts.
 */
#define taskENABLE_INTERRUPTS() portENABLE_INTERRUPTS()


/**
 *  Starts the real time kernel tick processing.  After calling the kernel
 *  has control over which tasks are executed and when.  This function
 *  does not return until an executing task calls vTaskEndScheduler().
 *
 *  At least one task should be created via a call to xTaskCreate()
 *  before calling vTaskStartScheduler().  The idle task is created
 *  automatically when the first application task is created.
 *
 *  See the demo application file main.c for an example of creating
 *  tasks and starting the kernel.
 *
 *  <b>Example usage:</b>
 *  @code
 *  void vAFunction( void )
 *  {
 *      // Create at least one task before starting the kernel.
 *      xTaskCreate( vTaskCode, "NAME", STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL );
 *
 *      // Start the real time kernel with preemption.
 *      vTaskStartScheduler();
 *
 *      // Will not get here unless a task calls vTaskEndScheduler()
 *  }
 *  @endcode
 */
void vTaskStartScheduler( void );


/**
 *  Stops the real time kernel tick.  All created tasks will be automatically
 *  deleted and multitasking (either preemptive or cooperative) will
 *  stop.  Execution then resumes from the point where vTaskStartScheduler()
 *  was called, as if vTaskStartScheduler() had just returned.
 *
 *  See the demo application file main. c in the demo/PC directory for an
 *  example that uses vTaskEndScheduler().
 *
 *  vTaskEndScheduler() requires an exit function to be defined within the
 *  portable layer (see vPortEndScheduler() in port. c for the PC port).  This
 *  performs hardware specific operations such as stopping the kernel tick.
 *
 *  vTaskEndScheduler() will cause all of the resources allocated by the
 *  kernel to be freed - but will not free resources allocated by application
 *  tasks.
 *
 *  <b>Example usage:</b>
 *  @code
 *  void vTaskCode( void * pvParameters )
 *  {
 *      for( ;; )
 *      {
 *          // Task code goes here.
 *
 *          // At some point we want to end the real time kernel processing
 *          // so call ...
 *          vTaskEndScheduler();
 *      }
 *  }
 *
 *  void vAFunction( void )
 *  {
 *      // Create at least one task before starting the kernel.
 *      xTaskCreate( vTaskCode, "NAME", STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL );
 *
 *      // Start the real time kernel with preemption.
 *      vTaskStartScheduler();
 *
 *      // Will only get here when the vTaskCode() task has called
 *      // vTaskEndScheduler().  When we get here we are back to single task
 *      // execution.
 *  }
 *  @endcode
 */
void vTaskEndScheduler( void );


/**
 *  Suspends all real time kernel activity while keeping interrupts (including the
 *  kernel tick) enabled.
 *
 *  After calling vTaskSuspendAll() the calling task will continue to execute
 *  without risk of being swapped out until a call to xTaskResumeAll() has been
 *  made.
 *
 *  API functions that have the potential to cause a context switch (for example, 
 *  vTaskDelayUntil(), xQueueSend(), etc.) must not be called while the scheduler 
 *  is suspended.
 *
 *  <b>Example usage:</b>
 *      @code
 *  void vTask1( void * pvParameters )
 *  {
 *      for( ;; )
 *      {
 *          // Task code goes here.
 *
 *          // ...
 *
 *          // At some point the task wants to perform a long operation during
 *          // which it does not want to get swapped out.  It cannot use
 *          // taskENTER_CRITICAL()/taskEXIT_CRITICAL() as the length of the
 *          // operation may cause interrupts to be missed - including the
 *          // ticks.
 *
 *          // Prevent the real time kernel swapping out the task.
 *          vTaskSuspendAll();
 *
 *          // Perform the operation here.  There is no need to use critical
 *          // sections as we have all the microcontroller processing time.
 *          // During this time interrupts will still operate and the kernel
 *          // tick count will be maintained.
 *
 *          // ...
 *
 *          // The operation is complete.  Restart the kernel.
 *          xTaskResumeAll();
 *      }
 *  }
 *  @endcode
 */
void vTaskSuspendAll( void );


/**
 *  Resumes real time kernel activity following a call to vTaskSuspendAll().
 *  After a call to vTaskSuspendAll() the kernel will take control of which
 *  task is executing at any time.
 *
 *  @return If resuming the scheduler caused a context switch then pdTRUE is
 *          returned, otherwise pdFALSE is returned.
 *
 *  <b>Example usage:</b>
 *  @code
 *  void vTask1( void * pvParameters )
 *  {
 *      for( ;; )
 *      {
 *          // Task code goes here.
 *
 *          // ...
 *
 *          // At some point the task wants to perform a long operation during
 *          // which it does not want to get swapped out.  It cannot use
 *          // taskENTER_CRITICAL()/taskEXIT_CRITICAL() as the length of the
 *          // operation may cause interrupts to be missed - including the
 *          // ticks.
 *
 *          // Prevent the real time kernel swapping out the task.
 *          vTaskSuspendAll();
 *
 *          // Perform the operation here.  There is no need to use critical
 *          // sections as we have all the microcontroller processing time.
 *          // During this time interrupts will still operate and the real
 *          // time kernel tick count will be maintained.
 *
 *          // ...
 *
 *          // The operation is complete.  Restart the kernel.  We want to force
 *          // a context switch - but there is no point if resuming the scheduler
 *          // caused a context switch already.
 *          if( !xTaskResumeAll() )
 *          {
 *              taskYIELD();
 *          }
 *      }
 *  }
 *  @endcode
 */
signed portBASE_TYPE xTaskResumeAll( void );

/**
 *  Utility task that simply returns pdTRUE if the task referenced by xTask is
 *  currently in the Suspended state, or pdFALSE if the task referenced by xTask
 *  is in any other state.
 *
 *  @param xTask Handle to the task of interest
 *
 *  @return If resuming the scheduler caused a context switch then pdTRUE is
 *          returned, otherwise pdFALSE is returned.
 */
signed portBASE_TYPE xTaskIsTaskSuspended( xTaskHandle xTask );
/** @} */


/* -------------------------------------------------------------------------- */
/** \defgroup TaskUtils Task Utilities API
 *  @{ */
/* -------------------------------------------------------------------------- */


/**
 *  Defines the priority used by the idle task.  This must not be modified.
 */
#define tskIDLE_PRIORITY ((unsigned portBASE_TYPE) 0)

#if (INCLUDE_xTaskGetCurrentTaskHandle == 1)
/*
 * Return the handle of the calling task.
 */
xTaskHandle xTaskGetCurrentTaskHandle( void );
#endif

/**
 *  Get the count of ticks since vTaskStartScheduler was called.
 *
 *  @return The tick count.
 */
portTickType xTaskGetTickCount( void );


/**
 *  Get the number of tasks.
 *
 *  @return The number of tasks that the real time kernel is currently managing.
 *  This includes all ready, blocked and suspended tasks.  A task that
 *  has been deleted but not yet freed by the idle task will also be
 *  included in the count.
 */
unsigned portBASE_TYPE uxTaskGetNumberOfTasks( void );


#if ((1 == configUSE_TRACE_FACILITY) && (1 == INCLUDE_vTaskDelete) && (1 == INCLUDE_vTaskSuspend))
/**
 *  Lists all the current tasks, along with their current state and stack
 *  usage high water mark.
 *
 *  @note This function will disable interrupts for its duration.  It is
 *  not intended for normal application runtime use but as a debug aid.
 *
 *  Tasks are reported as blocked ('B'), ready ('R'), deleted ('D') or
 *  suspended ('S').
 *
 *  @param pcWriteBuffer A buffer into which the above mentioned details
 *  will be written, in ascii form.  This buffer is assumed to be large
 *  enough to contain the generated report.  Approximately 40 bytes per
 *  task should be sufficient.
 */
void vTaskList( signed portCHAR *pcWriteBuffer );
#endif


/**
 *  Starts a real time kernel activity trace.  The trace logs the identity of
 *  which task is running when.
 *
 *  The trace file is stored in binary format.  A separate DOS utility called
 *  convtrce.exe is used to convert this into a tab delimited text file which
 *  can be viewed and plotted in a spread sheet.
 *
 *  @param pcBuffer The buffer into which the trace will be written.
 *
 *  @param ulBufferSize The size of pcBuffer in bytes.  The trace will continue
 *  until either the buffer in full, or ulTaskEndTrace() is called.
 */
void vTaskStartTrace( signed portCHAR * pcBuffer, unsigned portLONG ulBufferSize );


/**
 *  Stops a kernel activity trace.  See vTaskStartTrace().
 *
 *  @return The number of bytes that have been written into the trace buffer.
 */
unsigned portLONG ulTaskEndTrace( void );

#if (1 == INCLUDE_uxTaskGetStackHighWaterMark)
/**
 *  Returns the high water mark of the stack associated with xTask.  That is,
 *  the minimum free stack space there has been (in bytes) since the task
 *  started.  The smaller the returned number the closer the task has come
 *  to overflowing its stack.
 *
 *  @note Set xTask to NULL to check the stack of the calling task.
 *
 *  @param xTask Handle of the task associated with the stack to be checked.
 *
 *  @return The smallest amount of free stack space there has been (in bytes)
 *          since the task referenced by xTask was created.
 */
unsigned portBASE_TYPE uxTaskGetStackHighWaterMark( xTaskHandle xTask );
#endif

/**
 *  Sets pxHookFunction to be the task hook function used by the task xTask.
 *
 *  @note Passing xTask as NULL has the effect of setting the calling tasks hook
 *  function.
 *
 *  @param xTask Handle of the task being assigned a hook function
 *  @param pxHookFunction The function to set as a callback
 */
void vTaskSetApplicationTaskTag( xTaskHandle xTask, pdTASK_HOOK_CODE pxHookFunction );

/**
 *  Calls the hook function associated with xTask.
 *
 *  @note Passing xTask as NULL has the effect of calling the Running tasks
 *  (the calling task) hook function.
 *
 *  @param xTask The task who's hook is being called
 *  @param pvParameter user data passed to the hook function for the task to
 *                     interpret as it wants
 */
portBASE_TYPE xTaskCallApplicationTaskHook( xTaskHandle xTask, void *pvParameter );


/** @} */


#ifdef __cplusplus
}
#endif


/** @} */


#endif /* TASK_H */
