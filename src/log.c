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
* @file log.c
* @brief
*/
#include "log.h"

// standard includes
#include <stdio.h>
#include <stdarg.h>

// FreeRTOS-Kernel includes
#include "task.h"
#include "queue.h"

// alert-panel includes
#include "system.h"

/**
 * @brief
 *
 */
#define LOG_MESSAGE_SIZE 256

/**
 * @brief
 *
 */
typedef char Message[LOG_MESSAGE_SIZE];

/**
 * @brief
 *
 */
static QueueHandle_t log_queue;

/**
 * @brief
 *
 * @param params
 */
static void LogTask(void *params);

/**
 * @brief
 *
 * @param level
 * @param module
 * @param fmt
 * @param args
 * @return int
 */
static int LogVargPrint(const char *level, const char *module, const char *fmt, va_list args);

/**
 * @brief
 *
 * @param level
 * @param module
 * @param fmt
 * @param args
 * @return int
 */
static int LogPrintMqtt(const char *level, const char *module, const char *fmt, va_list args);

/*-----------------------------------------------------------*/

int LogInit()
{
    log_queue = xQueueCreate(10, sizeof(Message));

    if (log_queue == NULL)
    {
        int core_id = portGET_CORE_ID();
        printf("[FATAL] [%s] [%d] Failed to create log_queue", __FILE__, core_id); // Need to printf here as logging not setup
        Fault();
    }
}

/*-----------------------------------------------------------*/

void LogTaskCreate(UBaseType_t priority, UBaseType_t core_affinity_mask)
{
    xTaskCreatePinnedToCore(LogTask, "LogTask", configMINIMAL_STACK_SIZE, NULL, priority, NULL, core_affinity_mask);
}

/*-----------------------------------------------------------*/

int LogPrint(const char *level, const char *module, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int result = LogVargPrint(level, module, fmt, args);
    va_end(args);
    return result;
}

/*-----------------------------------------------------------*/

static void LogTask(void *params)
{
    Message msg;

    for (;;)
    {
        // Wait for next message
        if (xQueueReceive(log_queue, &msg, portMAX_DELAY) != pdTRUE)
        {
            int core_id = portGET_CORE_ID();
            printf("[FATAL] [%s] [%d] log_queue receive failed", __FILE__, core_id);
            Fault();
        }

        printf(msg);
    }
}

/*-----------------------------------------------------------*/

static int LogVargPrint(const char *level, const char *module, const char *fmt, va_list args)
{
    Message msg;
    int core_id = portGET_CORE_ID();
    int bytes_written;

    bytes_written = snprintf(msg, sizeof(msg), "[%s] [%s] [%d] ", level, module, core_id);
    if (bytes_written < 0)
    {
        LogPrint("FATAL", __FILE__, "Log message build failed"); // This might not get through?
        Fault();
    }

    bytes_written += vsnprintf(msg + bytes_written, sizeof(msg) - bytes_written, fmt, args);

    if (bytes_written >= sizeof(msg))
    {
        msg[sizeof(msg) - 1] = '\0';
    }

    // Send message
    if (xQueueSend(log_queue, &msg, portMAX_DELAY) != pdTRUE)
    {
        LogPrint("FATAL", __FILE__, "Failed to send to log_queue"); // This might not get through?
        Fault();
    }

    return 0;
}

/*-----------------------------------------------------------*/

static int LogPrintMqtt(const char *level, const char *module, const char *fmt, va_list args)
{
    char fmt_nl[LOG_MESSAGE_SIZE];
    int bytes_written = vsnprintf(fmt_nl, sizeof(fmt_nl) - 2, fmt, args);
    int nl_pos = (bytes_written < sizeof(fmt_nl) - 2) ? bytes_written : sizeof(fmt_nl) - 2;
    fmt_nl[nl_pos] = '\n';
    fmt_nl[nl_pos + 1] = '\0';
    return LogPrint(level, module, fmt_nl);
}

/*-----------------------------------------------------------*/

int LogPrintMqttError(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int result = LogPrintMqtt("ERROR", "coreMQTT", fmt, args);
    va_end(args);
    return result;
}

/*-----------------------------------------------------------*/

int LogPrintMqttWarn(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int result = LogPrintMqtt("WARN", "coreMQTT", fmt, args);
    va_end(args);
    return result;
}

/*-----------------------------------------------------------*/

int LogPrintMqttInfo(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int result = LogPrintMqtt("INFO", "coreMQTT", fmt, args);
    va_end(args);
    return result;
}

/*-----------------------------------------------------------*/

int LogPrintMqttDebug(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int result = LogPrintMqtt("DEBUG", "coreMQTT", fmt, args);
    va_end(args);
    return result;
}