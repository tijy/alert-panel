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
* @file keypad_driver.c
* @brief Adapted from https://github.com/pimoroni/pimoroni-pico/tree/main/libraries/pico_rgb_keypad
*/
#include "keypad_driver.h"

// standard includes
#include <math.h>
#include <string.h>

// pico-sdk includes
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"

// keypad properties
#define KEYPAD_ADDRESS  0x20
#define WIDTH           4
#define HEIGHT          5
#define NUM_PADS        (WIDTH * HEIGHT)

// gpio pins
#define SDA     4
#define SCL     5
#define CS      17
#define SCK     18
#define MOSI    19

/**
 * @brief Brightness information for each led, used for restoration on state = ON
 *
 */
static float led_restore_brightness[NUM_PADS];

/**
 * @brief ON/OFF information for each led
 *
 */
static bool led_state[NUM_PADS];

/**
 * @brief Full led_buffer to be written to device
 *
 */
static uint8_t led_buffer[(NUM_PADS * 4) + 8];

/**
 * @brief Pointer to start of led colour/brightness information in led_buffer
 *
 */
static uint8_t *led_data;

/*-----------------------------------------------------------*/

void KeypadDriverInit(void)
{
    memset(led_buffer, 0, sizeof(led_buffer));
    led_data = led_buffer + 4;

    for (uint16_t i = 0; i < NUM_PADS; i++)
    {
        led_restore_brightness[i] = 0.5f;
        led_state[i] = true;
        KeypadDriverSetLedBrightness(i, 0.5f);
        KeypadDriverSetLedColour(i, 255, 255, 255);
    }

    i2c_init(i2c0, 400000);
    gpio_set_function(SDA, GPIO_FUNC_I2C);
    gpio_pull_up(SDA);
    gpio_set_function(SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SCL);
    spi_init(spi0, 4 * 1024 * 1024);
    gpio_set_function(CS, GPIO_FUNC_SIO);
    gpio_set_dir(CS, GPIO_OUT);
    gpio_put(CS, 1);
    gpio_set_function(SCK, GPIO_FUNC_SPI);
    gpio_set_function(MOSI, GPIO_FUNC_SPI);
    KeypadDriverFlush();

    for (uint16_t i = 0; i < NUM_PADS; i++)
    {
        KeypadDriverSetLedOff(i);
        KeypadDriverSetLedBrightness(i, 0.0f);
        KeypadDriverSetLedColour(i, 0, 0, 0);
    }

    KeypadDriverFlush();
}

/*-----------------------------------------------------------*/

void KeypadDriverSetLedBrightness(uint8_t i, float brightness)
{
    if (i < 0 || i >= NUM_PADS)
    {
        return;
    }

    if (brightness < 0.0f || brightness > 1.0f)
    {
        return;
    }

    // Set restore value in case we're OFF
    led_restore_brightness[i] = brightness;

    if (led_state[i]) // if we're ON, set brightness on device
    {
        led_data[i * 4] = 0b11100000 | (uint8_t)(brightness * (float)0b11111);
    }
}

/*-----------------------------------------------------------*/

void KeypadDriverSetLedColour(uint8_t i, uint8_t r, uint8_t g, uint8_t b)
{
    if (i < 0 || i >= NUM_PADS)
    {
        return;
    }

    uint16_t offset = i * 4;
    led_data[offset + 1] = b;
    led_data[offset + 2] = g;
    led_data[offset + 3] = r;
}

/*-----------------------------------------------------------*/

void KeypadDriverSetLedOn(uint8_t i)
{
    if (i < 0 || i >= NUM_PADS)
    {
        return;
    }

    // Note we're now ON
    led_state[i] = true;
    // Set brightness to restore value
    led_data[i * 4] = 0b11100000 | (uint8_t)(led_restore_brightness[i] * (float)0b11111);
}

/*-----------------------------------------------------------*/

void KeypadDriverSetLedOff(uint8_t i)
{
    if (i < 0 || i >= NUM_PADS)
    {
        return;
    }

    // Note we're now OFF
    led_state[i] = false;
    // Set brightness to 0.0f
    led_data[i * 4] = 0b11100000 | (uint8_t)(0.0f * (float)0b11111);
}

/*-----------------------------------------------------------*/

uint16_t KeypadDriverGetButtonStates(void)
{
    uint8_t i2c_read_buffer[2];
    uint8_t reg = 0;
    i2c_write_blocking(i2c0, KEYPAD_ADDRESS, &reg, 1, true);
    i2c_read_blocking(i2c0, KEYPAD_ADDRESS, i2c_read_buffer, 2, false);
    return ~((i2c_read_buffer[0]) | (i2c_read_buffer[1] << 8));
}

/*-----------------------------------------------------------*/

void KeypadDriverFlush(void)
{
    gpio_put(CS, 0);
    spi_write_blocking(spi0, led_buffer, sizeof(led_buffer));
    gpio_put(CS, 1);
}
