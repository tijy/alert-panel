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
* @file led_monitor.c
* @brief
*/
#include "led_monitor.h"

// standard includes
#include <string.h>
#include <stdio.h>

// FreeRTOS-Kernel includes
#include "FreeRTOS.h"

// alert-panel includes
#include "activity_led.h"
#include "keypad.h"
#include "led_msg.h"
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
 * @brief Monitors mqtt for keypad led state change messages and sets them accordingly
 *
 * @param params
 */
static void LedMonitorTask(void *params);

/**
 * @brief
 *
 */
static void LedMonitorConnect();

/**
 * @brief
 *
 */
static void LedMonitorPublishInitialStates();

/**
 * @brief
 *
 */
static void LedMonitorCommandReceive();

/*-----------------------------------------------------------*/

void LedMonitorTaskCreate(UBaseType_t priority, UBaseType_t core_affinity_mask)
{
    xTaskCreatePinnedToCore(LedMonitorTask, "LedMonitorTask", configMINIMAL_STACK_SIZE, &core_affinity_mask, priority, NULL,
                            core_affinity_mask);
}

/*-----------------------------------------------------------*/

static void LedMonitorTask(void *params)
{
    LogPrintInfo("LedMonitorTask running...\n");
    LedMonitorConnect();
    LedMonitorPublishInitialStates();
    ActivityLedSetOn();

    while (1)
    {
        LedMonitorCommandReceive();
    }
}

/*-----------------------------------------------------------*/

static void LedMonitorConnect()
{
    // 1) Prepare will message
    LedMsgBuildAvailableTopic(topic_buffer, MQTT_TOPIC_BUFFER_SIZE);
    LedMsgBuildAvailablePayload(false, payload_buffer, MQTT_PAYLOAD_BUFFER_SIZE);
    // 2) Connect to MQTT in the led monitor task so we can send initial light state updates to broker
    MqttSubmitConnect(true,
                      MQTT_KEEP_ALIVE,
                      MQTT_CLIENT_ID,
                      strlen(MQTT_CLIENT_ID),
                      MQTT_BROKER_USERNAME,
                      strlen(MQTT_BROKER_USERNAME),
                      MQTT_BROKER_PASSWORD,
                      strlen(MQTT_BROKER_PASSWORD),
                      topic_buffer,
                      strlen(topic_buffer),
                      payload_buffer,
                      strlen(payload_buffer),
                      MQTTQoS2,
                      true);
    // 3) Register subscription
    LedMsgBuildCmdTopic(topic_buffer, MQTT_TOPIC_BUFFER_SIZE);
    MqttSubmitSubscribe(topic_buffer, strlen(topic_buffer), MQTTQoS2);
    // 4) Publish online message
    LedMsgBuildAvailableTopic(topic_buffer, MQTT_TOPIC_BUFFER_SIZE);
    LedMsgBuildAvailablePayload(true, payload_buffer, MQTT_PAYLOAD_BUFFER_SIZE);
    MqttSubmitPublish(topic_buffer, strlen(topic_buffer), payload_buffer, strlen(payload_buffer), MQTTQoS2, true);
}

/*-----------------------------------------------------------*/

static void LedMonitorPublishInitialStates()
{
    // 1) Set up base parameter to send for all leds
    KeypadLedParams_t params;
    memset(&params, 0, sizeof(KeypadLedParams_t));
    params.brightness_set = true;
    params.brightness = 0;
    params.colour_set = true;
    params.red = 0;
    params.green = 0;
    params.blue = 0;
    params.state_set = true;
    params.state = false;
    // 2) Build payload to send for all leds
    LedMsgBuildStatePayload(&params, payload_buffer, MQTT_PAYLOAD_BUFFER_SIZE);

    // 3) Submit for each led
    for (int index = 0; KEYPAD_KEYS < 16; index++)
    {
        params.key_id = KEYPAD_KEY_ID[index];
        LedMsgBuildStateTopic(&params, topic_buffer, MQTT_TOPIC_BUFFER_SIZE);
        MqttSubmitPublish(topic_buffer, strlen(topic_buffer), payload_buffer, strlen(payload_buffer), MQTTQoS2, true);
    }
}

/*-----------------------------------------------------------*/

static void LedMonitorCommandReceive()
{
    // 1) Wait for mqtt message
    message = MqttSubscriptionReceive();
    // 2) Clear parameters
    KeypadLedParams_t params;
    memset(&params, 0, sizeof(KeypadLedParams_t));

    // 3) Parse topic
    if (!LedMsgParseCmdTopic(&params, message.topic.data, message.topic.length))
    {
        LogPrintWarn("Failed to parse led cmd topic, ignoring message\n");
        return;
    }

    // 4) Parse payload
    if (!LedMsgParseCmdPayload(&params, message.payload.data, message.payload.length))
    {
        LogPrintWarn("Failed to parse led cmd payload, ignoring message\n");
        return;
    }

    // 5) Send parameters to be written to the keypad
    KeypadLedEventQueueSend(&params);
    // 6) Build led state topic
    LedMsgBuildStateTopic(&params, topic_buffer, MQTT_TOPIC_BUFFER_SIZE);
    // 7) Build led state PAYLOAD
    LedMsgBuildStatePayload(&params, payload_buffer, MQTT_PAYLOAD_BUFFER_SIZE);
    // 8) Publish updated state
    MqttSubmitPublish(topic_buffer,
                      strlen(topic_buffer),
                      payload_buffer,
                      strlen(payload_buffer),
                      MQTTQoS2,
                      true);
}
