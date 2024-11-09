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
* @file keypad.c
* @brief
*/
#include "keypad.h"

// FreeRTOS-Kernel includes
#include "task.h"
#include "queue.h"

// alert-panel includes
#include "keypad_driver.h"
#include "system.h"
#include "log.h"

/**
 * @brief Incoming led parameter messages
 *
 */
static QueueHandle_t params_queue;

/**
 * @brief Outgoing button event messages
 *
 */
static QueueHandle_t button_event_queue;

/**
 * @brief
 *
 * @param params
 */
static void KeypadTask(void *params);

/**
 * @brief Receive submitted led paramerts to be written to the device
 *
 * @param ticks_to_wait
 */
static void KeypadLedParamsReceive(TickType_t ticks_to_wait);

/**
 * @brief Processes a single led parameter and calls approproate keypad driver functions to write to device
 *
 * @param params
 */
static void KeypadProcessLedParams(KeypadLedParams_t *params);

/**
 * @brief Gets the key index from id (key ids are oriented differently to the pcb layout)
 *
 * @param key_id
 * @return uint8_t
 */
static uint8_t KeypadIndexFromKeyId(char key_id);

/**
 * @brief Converts a 0-255 uint value to a 0.0f-1.0f float value
 *
 * @param in
 * @return float
 */
static float KeypadUint8ToBrightnessFloat(uint8_t in);

/*-----------------------------------------------------------*/

int KeypadInit()
{
    params_queue = xQueueCreate(20, sizeof(KeypadLedParams_t));

    if (params_queue == NULL)
    {
        LogPrintFatal("Failed to create params_queue");
        Fault();
    }
}

/*-----------------------------------------------------------*/

void KeypadTaskCreate(UBaseType_t priority, UBaseType_t core_affinity_mask)
{
    xTaskCreatePinnedToCore(KeypadTask, "KeypadTask", configMINIMAL_STACK_SIZE, NULL, priority, NULL, core_affinity_mask);
}

/*-----------------------------------------------------------*/

static void KeypadTask(void *params)
{
    LogPrintInfo("KeypadTask running...\n");
    // Initialise keypad driver
    KeypadDriverInit();

    while (1)
    {
        // 1) Process any led set events that have been queued
        TickType_t ticks_to_wait = pdMS_TO_TICKS(1000);
        KeypadLedParamsReceive(ticks_to_wait);
        // 2) Process any button change events
        // Todo
    }
}

/*-----------------------------------------------------------*/

void KeypadLedParamsSend(KeypadLedParams_t *params)
{
    if (xQueueSend(params_queue, params, portMAX_DELAY) != pdTRUE)
    {
        LogPrintFatal("Failed to send to params_queue");
        Fault();
    }
}

/*-----------------------------------------------------------*/

static void KeypadLedParamsReceive(TickType_t ticks_to_wait)
{
    KeypadLedParams_t params;
    bool flush_needed = false;

    while (1)
    {
        if (xQueueReceive(params_queue, &params, ticks_to_wait) == pdTRUE)
        {
            KeypadProcessLedParams(&params);
            flush_needed = true;
            ticks_to_wait = 0; // Try to get more data from the queue if it exists,
        }
        else
        {
            // we timed out
            break;
        }
    }

    if (flush_needed)
    {
        KeypadDriverFlush();
    }
}

/*-----------------------------------------------------------*/

static void KeypadProcessLedParams(KeypadLedParams_t *params)
{
    uint8_t key_index = KeypadIndexFromKeyId(params->key_id);
    LogPrintDebug("Setting LED parameters...\n");
    LogPrintDebug("params->key_id: %c\n", params->key_id);
    LogPrintDebug("params->brightness_set: %i\n", params->brightness_set);
    LogPrintDebug("params->colour_set: %i\n", params->colour_set);
    LogPrintDebug("params->effect_set: %i\n", params->effect_set);
    LogPrintDebug("params->state_set: %i\n", params->state_set);
    LogPrintDebug("params->brightness: %u\n", params->brightness);
    LogPrintDebug("params->red: %u\n", params->red);
    LogPrintDebug("params->green: %u\n", params->green);
    LogPrintDebug("params->blue: %u\n", params->blue);
    LogPrintDebug("params->state %i\n", params->state);
    LogPrintDebug("key_index: %u\n", key_index);

    // 1) Led state (ON/OFF)
    if (params->state_set)
    {
        if (params->state) // ON
        {
            KeypadDriverSetLedOn(key_index);
        }
        else // OFF
        {
            KeypadDriverSetLedOff(key_index);
        }
    }

    // 2) Led colour (r,g,b)
    if (params->colour_set)
    {
        KeypadDriverSetLedColour(key_index, params->red, params->green, params->blue);
    }

    // 3) Led brightness
    if (params->brightness_set)
    {
        float brightness = KeypadUint8ToBrightnessFloat(params->brightness);
        KeypadDriverSetLedBrightness(key_index, brightness);
    }
}

/*-----------------------------------------------------------*/

static uint8_t KeypadIndexFromKeyId(char key_id)
{
    switch (key_id)
    {
        case '0':
            return 0x03;

        case '1':
            return 0x07;

        case '2':
            return 0x0b;

        case '3':
            return 0x0f;

        case '4':
            return 0x02;

        case '5':
            return 0x06;

        case '6':
            return 0x0a;

        case '7':
            return 0x0e;

        case '8':
            return 0x01;

        case '9':
            return 0x05;

        case 'a':
            return 0x09;

        case 'b':
            return 0x0d;

        case 'c':
            return 0x00;

        case 'd':
            return 0x04;

        case 'e':
            return 0x08;

        case 'f':
            return 0x0c;

        default:
            LogPrintFatal("Invalid key_id: %c", key_id);
            Fault();
            break;
    }
}

/*-----------------------------------------------------------*/

static float KeypadUint8ToBrightnessFloat(uint8_t in)
{
    if (in == 0)
    {
        return 0.0f; // No funny business on edge cases
    }

    if (in == 255)
    {
        return 1.0f; // No funny business on edge cases
    }

    return ((float)in / (float)255);
}
