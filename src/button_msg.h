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
* @file button_msg.h
* @brief Public functions in this module file are NOT thread-safe
*/
#ifndef _BUTTON_MSG_H
#define _BUTTON_MSG_H

// standard includes
#include <stdint.h>

// alert-panel includes
#include "keypad.h"

/**
 * @brief
 *
 * @param params
 * @param topic_buffer
 * @param buffer_size
 */
void ButtonMsgBuildStateTopic(KeypadButtonParams_t *params, char *topic_buffer, size_t buffer_size);

/**
 * @brief
 *
 * @param params
 * @param payload_buffer
 * @param buffer_size
 */
void ButtonMsgBuildStatePayload(KeypadButtonParams_t *params, char *payload_buffer, size_t buffer_size);

#endif //_BUTTON_MSG_H