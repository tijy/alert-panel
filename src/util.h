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
* @file util.h
* @brief Public functions in this module file are thread-safe
*/
#ifndef _UTIL_H
#define _UTIL_H

// standard includes
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Converts a byte array to hex string (for log purposes)
 * hex_length must == (buffer_length * 3) + 1
 *
 * @param output_hex
 * @param output_hex_length
 * @param buffer
 * @param buffer_length
 */
void BytesToHex(char *output_hex, size_t output_hex_length, const char *buffer, const size_t buffer_length);

/**
 * @brief Get the current time since start in ms
 *
 * @return uint32_t
 */
uint32_t GetTimeMs(void);

/**
 * @brief
 *
 */
uint32_t GetElapsedMs(uint32_t earlier, uint32_t later);

#endif //_UTIL_H