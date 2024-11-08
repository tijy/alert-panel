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
* @file log.h
* @brief Public functions in this module file are thread-safe
*/
#ifndef _LOG_H
#define _LOG_H

// FreeRTOS-Kernel includes
#include "FreeRTOS.h"

typedef enum
{
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
}
LogLevel_t;

/**
 * @brief
 *
 * @return int
 */
int LogInit();

/**
 * @brief
 *
 * @param priority
 * @param core_affinity_mask
 */
void LogTaskCreate(UBaseType_t priority, UBaseType_t core_affinity_mask);

/**
 * @brief
 *
 * @param level
 * @param module
 * @param fmt
 * @param ...
 * @return int
 */
int LogPrint(const char *level, const char *module, const char *fmt, ...);

/**
 * @brief
 *
 * @param fmt
 * @param ...
 * @return int
 */
int LogPrintMqttError(const char *fmt, ...);

/**
 * @brief
 *
 * @param fmt
 * @param ...
 * @return int
 */
int LogPrintMqttWarn(const char *fmt, ...);

/**
 * @brief
 *
 * @param fmt
 * @param ...
 * @return int
 */
int LogPrintMqttInfo(const char *fmt, ...);

/**
 * @brief
 *
 * @param fmt
 * @param ...
 * @return int
 */
int LogPrintMqttDebug(const char *fmt, ...);

#endif //_LOG_H