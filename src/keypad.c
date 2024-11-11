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

// standard includes
#include <string.h>

// FreeRTOS-Kernel includes
#include "task.h"
#include "queue.h"

// alert-panel includes
#include "keypad_driver.h"
#include "system.h"
#include "log.h"
#include "util.h"

/**
 * @brief
 *
 */
#define KEYPAD_BUTTON_HOLD_DURATION  800

/**
 * @brief
 *
 */
#define KEYPAD_POLL_PERIOD  10

/**
 * @brief
 *
 */
const char KEYPAD_KEY_ID[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

/**
 * @brief
 *
 */
const uint8_t KEYPAD_KEY_INDEX[] = {0x03, 0x07, 0x0b, 0x0f, 0x02, 0x06, 0x0a, 0x0e, 0x01, 0x05, 0x09, 0x0d, 0x00, 0x04, 0x08, 0x0c};

/**
 * @brief
 *
 */
typedef enum
{
    RELEASED = 0,
    PRESSED = 1,
    HELD = 2
}
KeyButtonState_t;

/**
 * @brief
 *
 */
typedef struct
{
    KeyButtonState_t last_state;
    uint32_t last_updated;
}
KeyButtonPollState_t;

/**
 * @brief
 *
 */
static KeyButtonPollState_t last_button_state[KEYPAD_KEYS];

/**
 * @brief Incoming led parameter messages
 *
 */
static QueueHandle_t led_event_queue;

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
static void KeypadLedEventQueueReceive(TickType_t ticks_to_wait);

/**
 * @brief Processes a single led parameter and calls approproate keypad driver functions to write to device
 *
 * @param params
 */
static void KeypadProcessLedEvent(KeypadLedParams_t *params);

/**
 * @brief
 *
 */
static void KeypadButtonStatePoll();

/**
 * @brief
 *
 * @param params
 */
static void KeypadButtonEventQueueSend(KeypadButtonParams_t *params);

/**
 * @brief Gets the key index from id (key ids are oriented differently to the pcb layout)
 *
 * @param key_id
 * @return uint8_t
 */
static uint8_t KeypadIndexFromId(char key_id);

/**
 * @brief
 *
 * @param key_index
 * @return char
 */
static char KeypadIdFromIndex(uint8_t key_index);

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
    // Clear button states
    memset(last_button_state, 0, sizeof(last_button_state));
    led_event_queue = xQueueCreate(20, sizeof(KeypadLedParams_t));

    if (led_event_queue == NULL)
    {
        LogPrintFatal("Failed to create led_event_queue");
        Fault();
    }

    button_event_queue = xQueueCreate(20, sizeof(KeypadButtonParams_t));

    if (button_event_queue == NULL)
    {
        LogPrintFatal("Failed to create button_event_queue");
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
    TickType_t ticks_to_wait = pdMS_TO_TICKS(KEYPAD_POLL_PERIOD);

    while (1)
    {
        // 1) Process any led set events that have been queued
        KeypadLedEventQueueReceive(ticks_to_wait);
        // 2) Process any button change events
        KeypadButtonStatePoll();
    }
}

/*-----------------------------------------------------------*/

void KeypadLedEventQueueSend(KeypadLedParams_t *params)
{
    if (xQueueSend(led_event_queue, params, portMAX_DELAY) != pdTRUE)
    {
        LogPrintFatal("Failed to send to led_event_queue");
        Fault();
    }
}

/*-----------------------------------------------------------*/

KeypadButtonParams_t KeypadButtonEventQueueReceive()
{
    KeypadButtonParams_t params;

    if (xQueueReceive(button_event_queue, &params, portMAX_DELAY) != pdTRUE)
    {
        LogPrintFatal("button_event_queue receive failed\n");
        Fault();
    }

    return params;
}

/*-----------------------------------------------------------*/

static void KeypadLedEventQueueReceive(TickType_t ticks_to_wait)
{
    KeypadLedParams_t params;
    bool flush_needed = false;

    while (1)
    {
        if (xQueueReceive(led_event_queue, &params, ticks_to_wait) == pdTRUE)
        {
            KeypadProcessLedEvent(&params);
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

static void KeypadProcessLedEvent(KeypadLedParams_t *params)
{
    uint8_t key_index = KeypadIndexFromId(params->key_id);
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

static void KeypadButtonStatePoll()
{
    uint16_t driver_button_states = KeypadDriverGetButtonStates();
    uint32_t time_now = GetTimeMs();

    for (int key_index = 0; key_index < KEYPAD_KEYS; key_index++)
    {
        bool is_pressed = (driver_button_states >> key_index) & 1; // Each bit represents the pressed/released state
        KeyButtonState_t *last_state = &(last_button_state[key_index].last_state);
        uint32_t *last_updated = &(last_button_state[key_index].last_updated);

        switch (*last_state)
        {
            case RELEASED:
                if (is_pressed)
                {
                    *last_state = PRESSED;
                    *last_updated = time_now;
                }
                else // !is_pressed
                {
                    // Button is just sittng happily idle
                }

                break;

            case PRESSED:
                if (is_pressed)
                {
                    uint32_t pressed_time = GetElapsedMs(*last_updated, time_now);

                    if (pressed_time >= KEYPAD_BUTTON_HOLD_DURATION)
                    {
                        *last_state = HELD;
                        *last_updated = time_now;
                        // Queue a hold event
                        KeypadButtonParams_t params;
                        params.key_id = KeypadIdFromIndex(key_index);
                        params.event = HOLD;
                        KeypadButtonEventQueueSend(&params);
                    }
                    else // pressed_time < KEYPAD_BUTTON_HOLD_DURATION
                    {
                        // Button is still being held down, but not long enough to be
                        // considered a 'hold'... wait for its eventual release
                    }
                }
                else // !is_pressed
                {
                    *last_state = RELEASED;
                    *last_updated = time_now;
                    // Queue a press event
                    KeypadButtonParams_t params;
                    params.key_id = KeypadIdFromIndex(key_index);
                    params.event = PRESS;
                    KeypadButtonEventQueueSend(&params);
                }

                break;

            case HELD:
                if (is_pressed)
                {
                    // Button is still being held down, but we've already sent an event
                }
                else // !is_pressed
                {
                    *last_state = RELEASED;
                    *last_updated = time_now;
                }

                break;

            default:
                break;
        }
    }
}

/*-----------------------------------------------------------*/

static void KeypadButtonEventQueueSend(KeypadButtonParams_t *params)
{
    if (xQueueSend(button_event_queue, params, portMAX_DELAY) != pdTRUE)
    {
        LogPrintFatal("Failed to send to button_event_queue");
        Fault();
    }
}

/*-----------------------------------------------------------*/

static uint8_t KeypadIndexFromId(char key_id)
{
    for (uint8_t i = 0; i < KEYPAD_KEYS; ++i)
    {
        if (KEYPAD_KEY_ID[i] == key_id)
        {
            return KEYPAD_KEY_INDEX[i];
        }
    }

    LogPrintFatal("Invalid key id: %c", key_id);
    Fault();
    return 0;
}

/*-----------------------------------------------------------*/

static char KeypadIdFromIndex(uint8_t key_index)
{
    for (uint8_t i = 0; i < KEYPAD_KEYS; ++i)
    {
        if (KEYPAD_KEY_INDEX[i] == key_index)
        {
            return KEYPAD_KEY_ID[i];
        }
    }

    LogPrintFatal("Invalid key index: %u", key_index);
    Fault();
    return 0;
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
