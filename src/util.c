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
* @file util.c
* @brief
*/
#include "util.h"

// standard includes
#include <stdio.h>

// FreeRTOS-Kernel includes
#include "FreeRTOS.h"
#include "task.h"

/*-----------------------------------------------------------*/

void BytesToHex(char *output_hex, size_t output_hex_length, const char *buffer, const size_t buffer_length)
{
    for (size_t i = 0; i < buffer_length; i++)
    {
        output_hex += sprintf(output_hex, "%02X ", buffer[i]);
    }
}

/*-----------------------------------------------------------*/

uint32_t GetTimeMs(void)
{
    // Implement a platform-specific way to return current time in milliseconds.
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

/*-----------------------------------------------------------*/

uint32_t GetElapsedMs(uint32_t earlier, uint32_t later)
{
    return later - earlier;
}