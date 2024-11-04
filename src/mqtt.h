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
* @file mqtt.h
* @brief
*/
#ifndef _MQTT_H
#define _MQTT_H

// standard includes
#include <stdint.h>
#include <stdbool.h>

// FreeRTOS-Kernel includes
#include "FreeRTOS.h"

// coreMQTT includes
#include "core_mqtt.h"

// alert-panel includes
#include "alert_panel_config.h"

/**
 * @brief
 *
 */
typedef struct
{
    char data[MQTT_TOPIC_BUFFER_SIZE];
    size_t length;
}
MqttTopic_t;


/**
 * @brief
 *
 */
typedef struct
{
    char data[MQTT_PAYLOAD_BUFFER_SIZE];
    size_t length;
}
MqttPayload_t;


/**
 * @brief
 *
 */
typedef struct
{
    MqttTopic_t topic;
    MqttPayload_t payload;
}
MqttMessage_t;

/**
 * @brief
 *
 */
void MqttInit(void);

/**
 * @brief
 *
 * @param priority
 * @param core_affinity_mask
 */
void MqttTaskCreate(UBaseType_t priority, UBaseType_t core_affinity_mask);

/**
 * @brief
 *
 * @param clean_session
 * @param keep_alive
 * @param client_id
 * @param client_id_length
 * @param username
 * @param username_length
 * @param password
 * @param password_length
 * @param will_topic
 * @param will_topic_length
 * @param will_payload
 * @param will_payload_length
 * @param will_qos
 * @param will_retain
 */
void MqttSubmitConnect(bool clean_session,
                       uint16_t keep_alive,
                       const char * client_id,
                       uint16_t client_id_length,
                       const char * username,
                       uint16_t username_length,
                       const char * password,
                       uint16_t password_length,
                       const char * will_topic,
                       size_t will_topic_length,
                       const char * will_payload,
                       size_t will_payload_length,
                       MQTTQoS_t will_qos,
                       bool will_retain);

/**
 * @brief
 *
 * @param topic
 * @param topic_length
 * @param payload
 * @param payload_length
 * @param qos
 * @param retain
 */
void MqttSubmitPublish(const char * topic,
                       size_t topic_length,
                       const char * payload,
                       size_t payload_length,
                       MQTTQoS_t qos,
                       bool retain);

/**
 * @brief
 *
 * @param topic
 * @param topic_length
 * @param qos
 */
void MqttSubmitSubscribe(const char * topic,
                         size_t topic_length,
                         MQTTQoS_t qos);

/**
 * @brief
 *
 * @return MqttMessage_t
 */
MqttMessage_t MqttSubscriptionReceive();

#endif //_MQTT_H