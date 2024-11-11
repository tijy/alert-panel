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
* @file button_monitor.c
* @brief
*/
#include "button_monitor.h"

// standard includes
#include <string.h>
#include <stdio.h>

// FreeRTOS-Kernel includes
#include "FreeRTOS.h"

// alert-panel includes
#include "keypad.h"
#include "button_msg.h"
#include "system.h"
#include "mqtt.h"
#include "log.h"
#include "alert_panel_config.h"

/**
 * @brief
 *
 */
static MqttMessage_t message;

/**
 * @brief
 *
 */
static char payload_buffer[MQTT_PAYLOAD_BUFFER_SIZE];

/**
 * @brief
 *
 */
static char topic_buffer[MQTT_TOPIC_BUFFER_SIZE];

/**
 * @brief Monitors mqtt for keypad button events and publishes them via mqtt
 *
 * @param params
 */
static void ButtonMonitorTask(void *params);

/*-----------------------------------------------------------*/

void ButtonMonitorTaskCreate(UBaseType_t priority, UBaseType_t core_affinity_mask)
{
    xTaskCreatePinnedToCore(ButtonMonitorTask, "ButtonMonitorTask", configMINIMAL_STACK_SIZE, &core_affinity_mask, priority, NULL,
                            core_affinity_mask);
}

/*-----------------------------------------------------------*/

static void ButtonMonitorTask(void *params)
{
    LogPrintInfo("ButtonMonitorTask running...\n");

    while (1)
    {
        // Wait for a button press/hold event
        KeypadButtonParams_t params = KeypadButtonEventQueueReceive();
        LogPrintDebug("Received keypad button event, id:%c, e:%u\n", params.key_id, params.event);
        ButtonMsgBuildStateTopic(&params, topic_buffer, MQTT_TOPIC_BUFFER_SIZE);
        ButtonMsgBuildStatePayload(&params, payload_buffer, MQTT_PAYLOAD_BUFFER_SIZE);
        MqttSubmitPublish(topic_buffer, strlen(topic_buffer), payload_buffer, strlen(payload_buffer), MQTTQoS2, false);
    }
}
