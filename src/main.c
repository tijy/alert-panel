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
#include "wifi.h"
#include "alert_panel_config.h"

// Core 0 priorities
#define PRIORITY_LAUNCH ( tskIDLE_PRIORITY + 1U )
#define PRIORITY_LED    ( tskIDLE_PRIORITY + 1U )
#define PRIORITY_LOG    ( tskIDLE_PRIORITY + 3U )

// Core 1 priorities
#define PRIORITY_MQTT   ( tskIDLE_PRIORITY + 1U )

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
    ActivityLedTaskCreate(PRIORITY_LED, AFFINITY_CORE_0);

    // 3) Connect wifi
    WifiConnect(WIFI_SSID, WIFI_PASSWORD);

    // 4) Start mqtt service task
    MqttInit();
    MqttTaskCreate(PRIORITY_MQTT, AFFINITY_CORE_1);

    // 5) Connect to MQTT broker

    // 5) Start button monitoring

    // 6) Start led monitoring

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

    // Allow for usb connection for debugging purposes
    sleep_ms(5000);
    printf("Starting alert-panel...\n");

    // Create launch task
    xTaskCreatePinnedToCore(launch_task, "LaunchTask", configMINIMAL_STACK_SIZE, NULL, PRIORITY_LAUNCH, NULL, AFFINITY_CORE_0);

    // Start scheduler
    vTaskStartScheduler();
}
