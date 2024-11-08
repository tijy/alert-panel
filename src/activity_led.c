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
* @file activity_led.c
* @brief
*/
#include "activity_led.h"

// standard includes
#include <stdbool.h>

// pico-sdk includes
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

// FreeRTOS-Kernel includes
#include "task.h"

// alert-panel includes
#include "log.h"
#include "system.h"

/**
 * @brief
 *
 */
#define LED_ON          ( 0xFFFFFFFF )

/**
 * @brief
 *
 */
#define LED_OFF         ( 0x00000000 )

/**
 * @brief
 *
 */
#define DEFAULT_DELAY   ( 10000U )

/**
 * @brief
 *
 */
static uint32_t activity_led_interval;

/**
 * @brief
 *
 */
static TaskHandle_t activity_led_task_handle;

/**
 * @brief
 *
 * @param params
 */
static void ActivityLedTask(void *params);

/*-----------------------------------------------------------*/

void ActivityLedInit()
{
    activity_led_interval = LED_OFF;
    activity_led_task_handle = NULL;
}

/*-----------------------------------------------------------*/

void ActivityLedTaskCreate(UBaseType_t priority, UBaseType_t core_affinity_mask)
{
    xTaskCreatePinnedToCore(ActivityLedTask, "ActivityLedTask", configMINIMAL_STACK_SIZE, NULL, priority, &activity_led_task_handle,
                            core_affinity_mask);
}

/*-----------------------------------------------------------*/

void ActivityLedSetFlash(uint32_t interval)
{
    xTaskNotify(activity_led_task_handle, interval, eSetValueWithOverwrite);
}

/*-----------------------------------------------------------*/

void ActivityLedSetOn()
{
    xTaskNotify(activity_led_task_handle, LED_ON, eSetValueWithOverwrite);
}

/*-----------------------------------------------------------*/

void ActivityLedSetOff()
{
    xTaskNotify(activity_led_task_handle, LED_OFF, eSetValueWithOverwrite);
}

/*-----------------------------------------------------------*/

static void ActivityLedTask(void *params)
{
    LogPrint("INFO", __FILE__, "ActivityLedTask running...\n");
    bool flash = true;
    uint32_t delay = DEFAULT_DELAY;
    uint32_t received_value;

    while (1)
    {
        // How long should we wait for notify timeout?
        // If the LED is permenantly on or off, we don't have
        // to cycle quickly to 'flash' the LED
        if (activity_led_interval == LED_ON || activity_led_interval == LED_OFF)
        {
            delay = DEFAULT_DELAY;
        }
        else
        {
            delay = activity_led_interval;
        }

        // Wait for notify (i.e. a request to change ON/OFF/flash state OR
        // a timeout for either flashing the LED or just refreshing ON/OFF)
        if (xTaskNotifyWait(0, 0xFFFFFFFF, &received_value, pdMS_TO_TICKS(delay)) == pdTRUE)
        {
            // Got a notification
            activity_led_interval = received_value;
            flash = true; // Reset flash to true so we start flashing with the LED on
        }

        if (activity_led_interval == LED_ON) // ON
        {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
        }
        else if (activity_led_interval == LED_OFF) // OFF
        {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
        }
        else
        {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, flash);
            flash = !flash;
        }
    }
}
