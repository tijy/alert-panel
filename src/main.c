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
* @file main.c
* @brief
*/
// standard includes
#include <stdio.h>

// pico-sdk includes
#include "pico/stdlib.h"

// FreeRTOS-Kernel includes
#include "FreeRTOS.h"
#include "task.h"

// alert-panel includes
#include "activity_led.h"
#include "system.h"
#include "mqtt.h"
#include "log.h"
#include "led_monitor.h"
#include "keypad.h"
#include "wifi.h"
#include "alert_panel_config.h"

// Core 0 priorities
#define PRIORITY_LAUNCH             ( tskIDLE_PRIORITY + 1U )
#define PRIORITY_ACTIVITY_LED       ( tskIDLE_PRIORITY + 2U )
#define PRIORITY_LOG                ( tskIDLE_PRIORITY + 3U )
#define PRIORITY_LED_MONITOR        ( tskIDLE_PRIORITY + 4U )
#define PRIORITY_BUTTON_MONITOR     ( tskIDLE_PRIORITY + 5U )
#define PRIORITY_KEYPAD             ( tskIDLE_PRIORITY + 6U ) // Most important, want timely polling of buttons

// Core 1 priorities
#define PRIORITY_MQTT               ( tskIDLE_PRIORITY + 1U ) // Dedicated core for comms

/**
 * @brief
 *
 * @param params
 */
static void launch_task(void *params)
{
    // 1) Initialise logging first so we get messages through
    LogInit();
    LogTaskCreate(PRIORITY_LOG, AFFINITY_CORE_0);
    // 2) Initialise wifi (this uses cyw43 which is also required for the activity led!)
    WifiInit();
    // 2) Initialise activity led so we get simple visual indication of progress
    ActivityLedInit();
    ActivityLedTaskCreate(PRIORITY_ACTIVITY_LED, AFFINITY_CORE_0);
    // 3) Connect wifi
    WifiConnect(WIFI_SSID, WIFI_PASSWORD);
    // 4) Start mqtt service task
    MqttInit();
    MqttTaskCreate(PRIORITY_MQTT, AFFINITY_CORE_1);
    // 5) Start keypad task
    KeypadInit();
    KeypadTaskCreate(PRIORITY_KEYPAD, AFFINITY_CORE_0);
    // 5) Start led & button monitoring
    //ButtonMonitorTaskCreate(PRIORITY_BUTTON_MONITOR, AFFINITY_CORE_0);
    LedMonitorTaskCreate(PRIORITY_LED_MONITOR, AFFINITY_CORE_0);

    // Watchdog for hangs
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief
 *
 * @return int
 */
int main(void)
{
    // Init stdio
    stdio_init_all();
#ifdef DEBUG
    sleep_ms(5000); // Allow for usb connection for debugging purposes
    printf("Debug build\n");
#endif
    printf("Starting alert-panel...\n");
    // Create launch task
    xTaskCreatePinnedToCore(launch_task, "LaunchTask", configMINIMAL_STACK_SIZE, NULL, PRIORITY_LAUNCH, NULL, AFFINITY_CORE_0);
    // Start scheduler
    vTaskStartScheduler();
}
