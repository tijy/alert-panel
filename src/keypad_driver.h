/* MIT License
 *
 * Copyright (c) 2021 Pimoroni Ltd
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
* @file keypad_driver.h
* @brief Adapted from https://github.com/pimoroni/pimoroni-pico/tree/main/libraries/pico_rgb_keypad
* Public functions in this module file are NOT thread-safe
*/
#ifndef _KEYPAD_DRIVER_H
#define _KEYPAD_DRIVER_H

// standard includes
#include <stdint.h>

/**
 * @brief Initialise keypad driver
 *
 */
void KeypadDriverInit(void);

/**
 * @brief Set the led brightness for an individual button
 *
 * @param i
 * @param brightness
 */
void KeypadDriverSetLedBrightness(uint8_t i, float brightness);

/**
 * @brief Set the led colour for an individual button
 *
 * @param i
 * @param r
 * @param g
 * @param b
 */
void KeypadDriverSetLedColour(uint8_t i, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Turn the led on for an individual button
 *
 * @param i
 */
void KeypadDriverSetLedOn(uint8_t i);

/**
 * @brief  Turn the led off for an individual button
 *
 * @param i
 */
void KeypadDriverSetLedOff(uint8_t i);

/**
 * @brief Get all current button states
 *
 * @return uint16_t
 */
uint16_t KeypadDriverGetButtonStates(void);

/**
 * @brief Write changed led values to the device
 *
 */
void KeypadDriverFlush(void);

#endif //_KEYPAD_DRIVER_H