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
* @file system.h
* @brief
*/
#ifndef _SYSTEM_H
#define _SYSTEM_H

// standard includes
#include <stdint.h>

// FreeRTOS-Kernel includes
#include "FreeRTOS.h"
#include "task.h"

#define AFFINITY_CORE_0 ( 1U )
#define AFFINITY_CORE_1 ( 2U )

/**
 * @brief Own implementation of xTaskCreatePinnedToCore as FreeRTOS pico doesn't support it natively
 *
 *
 * @param pvTaskCode
 * @param pcName
 * @param usStackDepth
 * @param pvParameters
 * @param uxPriority
 * @param pvCreatedTask
 * @param xCoreID
 * @return BaseType_t
 */
BaseType_t xTaskCreatePinnedToCore(
    TaskFunction_t pvTaskCode,
    const char * pcName,
    const uint32_t usStackDepth,
    void *pvParameters,
    UBaseType_t uxPriority,
    TaskHandle_t *pvCreatedTask,
    const BaseType_t xCoreID
);

/**
 * @brief
 *
 *
 */
void Fault();

#endif //_SYSTEM_H