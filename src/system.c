/* MIT License
 *
 * Copyright (c) 2024 tijy
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
* @file system.c
* @brief
*/
#include "system.h"

// pico-sdk includes
#include "pico/stdlib.h"
#include "hardware/watchdog.h"

// FreeRTOS-Kernel includes
#include "semphr.h"
#include "log.h"

/**
 * @brief
 *
 */
#define PRIORITY_FAULT   ( 0xFF )

/**
 * @brief
 *
 */
typedef struct
{
    // task to create
    TaskFunction_t task_code;
    const char *name;
    const uint32_t stack_depth;
    void *parameters;
    UBaseType_t priority;
    TaskHandle_t *created_task;
    const BaseType_t core_id;

    // process signaling
    SemaphoreHandle_t affinity_set_signal;
    SemaphoreHandle_t create_complete_signal;
}
TaskData_t;

/**
 * @brief
 *
 * @param params
 */
static void TaskCreatePinnedToCoreTask(void *params);

/**
 * @brief
 *
 * @param params
 */
static void Core0FaultTask(void *params);

/**
 * @brief
 *
 * @param params
 */
static void Core1FaultTask(void *params);

/*-----------------------------------------------------------*/

BaseType_t xTaskCreatePinnedToCore(
    TaskFunction_t pvTaskCode,
    const char *pcName,
    const uint32_t usStackDepth,
    void *pvParameters,
    UBaseType_t uxPriority,
    TaskHandle_t *pvCreatedTask,
    const BaseType_t xCoreID)
{
    // Create a temp task object if none was passed in
    if (pvCreatedTask == NULL)
    {
        static TaskHandle_t created_task;
        pvCreatedTask = &created_task;
    }

    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
    {
        xTaskCreate(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pvCreatedTask);
        vTaskCoreAffinitySet(*pvCreatedTask, xCoreID);
    }
    else
    {
        // Initialize TaskData_t with task information
        TaskData_t task_data =
        {
            .task_code = pvTaskCode,
            .name = pcName,
            .stack_depth = usStackDepth,
            .parameters = pvParameters,
            .priority = uxPriority,
            .created_task = pvCreatedTask,
            .core_id = xCoreID,
            .affinity_set_signal = xSemaphoreCreateBinary(),
            .create_complete_signal = xSemaphoreCreateBinary()
        };
        // Create a task to handle pinning
        TaskHandle_t pinning_task;
        xTaskCreate(TaskCreatePinnedToCoreTask, "TaskCreatePinnedToCoreTask", configMINIMAL_STACK_SIZE, &task_data, uxPriority, &pinning_task);
        // Temporarily suspend, set affinity, and resume on the correct core
        vTaskSuspend(pinning_task);
        vTaskCoreAffinitySet(pinning_task, xCoreID);
        vTaskResume(pinning_task);
        // Signal the helper task that it is running on the desired core
        xSemaphoreGive(task_data.affinity_set_signal);
        // Wait until task creation completes
        xSemaphoreTake(task_data.create_complete_signal, portMAX_DELAY);
        // Clean up
        vSemaphoreDelete(task_data.affinity_set_signal);
        vSemaphoreDelete(task_data.create_complete_signal);
    }

    return pdPASS;
}

/*-----------------------------------------------------------*/

static void TaskCreatePinnedToCoreTask(void *params)
{
    TaskData_t *task_data = (TaskData_t *) params;
    // Wait for mutex confirming the new affinity has been set and we're executing on desired core
    xSemaphoreTake(task_data->affinity_set_signal, portMAX_DELAY);
    taskENTER_CRITICAL(); // We're now executing on the desire core, prevent any new task we create from executing code
    xTaskCreate(task_data->task_code, task_data->name, task_data->stack_depth, task_data->parameters, task_data->priority,
                task_data->created_task);
    vTaskCoreAffinitySet(*(task_data->created_task), task_data->core_id);
    taskEXIT_CRITICAL(); // Allow the task to execute (this will happen immediately if it's priority is higher)
    xSemaphoreGive(task_data->create_complete_signal); // Notify that task has been created
    vTaskDelete( NULL );
}

/*-----------------------------------------------------------*/

static void Core0FaultTask(void *params)
{
    vTaskEnterCritical();
    uint64_t startTime = time_us_64();

    while ((time_us_64() - startTime) < 10000000)
    {
        // Spin wait for 10 seconds
    }

    watchdog_enable(1, 1);

    while (1)
    {
        // wait
    }

    vTaskExitCritical();
}

/*-----------------------------------------------------------*/

static void Core1FaultTask(void *params)
{
    vTaskEnterCritical();

    while (1)
    {
        // wait
    }

    vTaskExitCritical();
}

/*-----------------------------------------------------------*/

void Fault()
{
    vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for prior log messages to flush
    xTaskCreatePinnedToCore(Core0FaultTask, "Core0FaultTask", configMINIMAL_STACK_SIZE, NULL, PRIORITY_FAULT, NULL, AFFINITY_CORE_0);
    xTaskCreatePinnedToCore(Core1FaultTask, "Core1FaultTask", configMINIMAL_STACK_SIZE, NULL, PRIORITY_FAULT, NULL, AFFINITY_CORE_1);
}

/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    LogPrint("FATAL", __FILE__, "Stack overflow detected in: %s\n", pcTaskName);
    Fault();
}