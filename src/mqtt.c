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
* @file mqtt.c
* @brief
*/
#include "mqtt.h"

// standard includes
#include <string.h>
#include <stdio.h>
#include <math.h>

// pico-sdk includes
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/ip4_addr.h"
#include "lwip/sockets.h"

// FreeRTOS-Kernel includes
#include "task.h"
#include "queue.h"

// coreMQTT includes
#include "transport_interface.h"

// alert-panel includes
#include "activity_led.h"
#include "log.h"
#include "system.h"
#include "util.h"

/**
 * @brief
 *
 */
#define SEND_RECV_FAILED    (-1)

/**
 * @brief
 *
 */
struct NetworkContext
{
    int socket;
};

/**
 * @brief
 *
 */
typedef struct
{
    bool clean_session;
    uint16_t keep_alive;
    char client_id[MQTT_CLIENT_ID_BUFFER_SIZE];
    uint16_t client_id_length;
    char username[MQTT_USERNAME_BUFFER_SIZE];
    uint16_t username_length;
    char password[MQTT_PASSWORD_BUFFER_SIZE];
    uint16_t password_length;
    MqttMessage_t will_message;
    MQTTQoS_t will_qos;
    bool will_retain;
}
MqttConnectData_t;

/**
 * @brief
 *
 */
typedef struct
{
    MqttMessage_t message;
    MQTTQoS_t qos;
    bool retain;
}
MqttPublishData_t;

/**
 * @brief
 *
 */
typedef void (* MqttSubscriptionCallback_t )( MqttMessage_t *message );

/**
 * @brief
 *
 */
typedef struct
{
    MqttTopic_t topic;
    MQTTQoS_t qos;
    MqttSubscriptionCallback_t callback;
}
MqttSubscribeData_t;

/**
 * @brief
 *
 */
typedef enum
{
    CONNECT = 1,
    PUBLISH = 2,
    SUBSCRIBE = 3,
}
MqttCommandType_t;

/**
 * @brief
 *
 */
typedef struct
{
    MqttCommandType_t type;
    MqttConnectData_t connect;
    MqttPublishData_t publish;
    MqttSubscribeData_t subscribe;
}
MqttCommand_t;

/**
 * @brief
 *
 */
typedef enum
{
    NOT_CONNECTED = 1,
    CONNECTED = 2
}
MqttConnectionState_t;

// core mqtt
static uint8_t packet_buffer[MQTT_PACKET_BUFFER_SIZE];
MQTTPubAckInfo_t incoming_pub_record_buffer[MQTT_PUBLISH_LIST_SIZE];
MQTTPubAckInfo_t outgoing_pub_record_buffer[MQTT_PUBLISH_LIST_SIZE];
static MQTTContext_t mqtt_context;
static NetworkContext_t network_context;
static MQTTFixedBuffer_t network_buffer;
static TransportInterface_t transport_interface;

/**
 * @brief
 *
 */
static QueueHandle_t command_queue;

/**
 * @brief
 *
 */
static QueueHandle_t subscription_queue;

/**
 * @brief
 *
 */
static MqttConnectionState_t connection_state = NOT_CONNECTED;

/**
 * @brief
 *
 */
static void MqttTask();

/**
 * @brief
 *
 * @param will_topic
 * @param will_topic_length
 * @param will_payload
 * @param will_payload_length
 * @param will_qos
 * @param will_retain
 */
static void MqttConnect(const char *will_topic,
                        size_t will_topic_length,
                        const char *will_payload,
                        size_t will_payload_length,
                        MQTTQoS_t will_qos,
                        bool will_retain);

/**
 * @brief
 *
 */
static void MqttContextInit();

/**
 * @brief
 *
 * @param mqtt_context
 * @param packet_info
 * @param deserialized_info
 */
void MqttEventCallback(MQTTContext_t *mqtt_context,
                       MQTTPacketInfo_t *packet_info,
                       MQTTDeserializedInfo_t *deserialized_info);

/**
 * @brief
 *
 */
static void MqttTransportConnect();

/**
 * @brief
 *
 * @param network_context
 */
static void MqttTransportDisconnect(NetworkContext_t *network_context);

/**
 * @brief
 *
 * @param network_context
 * @param buffer
 * @param bytes_to_send
 * @return int32_t
 */
static int32_t MqttTransportSend(NetworkContext_t *network_context,
                                 const void *buffer,
                                 size_t bytes_to_send);

/**
 * @brief
 *
 * @param network_context
 * @param buffer
 * @param bytes_to_recv
 * @return int32_t
 */
static int32_t MqttTransportRecv(NetworkContext_t *network_context,
                                 void *buffer,
                                 size_t bytes_to_recv);

/**
 * @brief
 *
 * @param topic
 * @param topic_length
 * @param qos
 */
static void MqttSubscribe(const char *topic,
                          size_t topic_length,
                          MQTTQoS_t qos);

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
static void MqttPublish(const char *topic,
                        size_t topic_length,
                        const char *payload,
                        size_t payload_length,
                        MQTTQoS_t qos,
                        bool retain);

/*-----------------------------------------------------------*/

void MqttInit(void)
{
    memset(&network_context, 0, sizeof(network_context));
    memset(&transport_interface, 0, sizeof(transport_interface));
    memset(&network_buffer, 0, sizeof(network_buffer));
    memset(&mqtt_context, 0, sizeof(mqtt_context));
    LogPrint("INFO", __FILE__, "command_queue object size: %u\n", sizeof(MqttCommand_t));
    command_queue = xQueueCreate(20, sizeof(MqttCommand_t));

    if (command_queue == NULL)
    {
        LogPrint("FATAL", __FILE__, "Failed to create command_queue\n");
        Fault();
    }

    subscription_queue = xQueueCreate(20, sizeof(MqttMessage_t));

    if (subscription_queue == NULL)
    {
        LogPrint("FATAL", __FILE__, "Failed to create subscription_queue\n");
        Fault();
    }
}

/*-----------------------------------------------------------*/

void MqttTaskCreate(UBaseType_t priority, UBaseType_t core_affinity_mask)
{
    xTaskCreatePinnedToCore(MqttTask, "MqttTask", 8192, NULL, priority, NULL, core_affinity_mask);
}

/*-----------------------------------------------------------*/

static void MqttTask()
{
    LogPrint("INFO", __FILE__, "MqttTask running...\n");
    MqttCommand_t command;
    memset(&command, 0, sizeof(MqttCommand_t));
    /*
    LogPrint("INFO", __FILE__, "%s: %u\n", "type", command.type);
    LogPrint("INFO", __FILE__, "%s: %s\n", "connect.will_message.topic.data", command.connect.will_message.topic.data);
    LogPrint("INFO", __FILE__, "%s: %u\n", "connect.will_message.topic.length", command.connect.will_message.topic.length);
    LogPrint("INFO", __FILE__, "%s: %s\n", "connect.will_message.payload.data", command.connect.will_message.payload.data);
    LogPrint("INFO", __FILE__, "%s: %u\n", "connect.will_message.payload.length", command.connect.will_message.payload.length);
    LogPrint("INFO", __FILE__, "%s: %u\n", "connect.will_qos", command.connect.will_qos);
    LogPrint("INFO", __FILE__, "%s: %u\n", "connect.will_retain", command.connect.will_retain);
    LogPrint("INFO", __FILE__, "%s: %s\n", "publish.message.topic.data", command.publish.message.topic.data);
    LogPrint("INFO", __FILE__, "%s: %u\n", "publish.message.topic.length", command.publish.message.topic.length);
    LogPrint("INFO", __FILE__, "%s: %s\n", "publish.message.payload.data", command.publish.message.payload.data);
    LogPrint("INFO", __FILE__, "%s: %u\n", "publish.message.payload.length", command.publish.message.payload.length);
    LogPrint("INFO", __FILE__, "%s: %u\n", "publish.qos", command.publish.qos);
    LogPrint("INFO", __FILE__, "%s: %u\n", "publish.retain", command.publish.retain);
    LogPrint("INFO", __FILE__, "%s: %s\n", "subscribe.topic.data", command.subscribe.topic.data);
    LogPrint("INFO", __FILE__, "%s: %u\n", "subscribe.topic.length", command.subscribe.topic.length);
    LogPrint("INFO", __FILE__, "%s: %u\n", "subscribe.qos", command.subscribe.qos);
    */
    MQTTStatus_t status;
    TickType_t ticks_to_wait = pdMS_TO_TICKS(10);

    while (1)
    {
        if (connection_state == CONNECTED)
        {
            // Call process loop to do any timeout processing/qos operations
            //LogPrint("DEBUG", __FILE__, "Calling MQTT_ProcessLoop\n");
            status = MQTT_ProcessLoop(&mqtt_context);

            if (status != MQTTSuccess && status != MQTTNeedMoreBytes)
            {
                //LogPrint("FATAL", __FILE__, "MQTT_ProcessLoop failed with: %u\n", status);
                Fault();
            }
        }

        // Wait for next command
        if (xQueueReceive(command_queue, &command, ticks_to_wait) == pdTRUE)
        {
            /*
            LogPrint("INFO", __FILE__, "%s: %u\n", "type", command.type);
            LogPrint("INFO", __FILE__, "%s: %s\n", "connect.will_message.topic.data", command.connect.will_message.topic.data);
            LogPrint("INFO", __FILE__, "%s: %u\n", "connect.will_message.topic.length", command.connect.will_message.topic.length);
            LogPrint("INFO", __FILE__, "%s: %s\n", "connect.will_message.payload.data", command.connect.will_message.payload.data);
            LogPrint("INFO", __FILE__, "%s: %u\n", "connect.will_message.payload.length", command.connect.will_message.payload.length);
            LogPrint("INFO", __FILE__, "%s: %u\n", "connect.will_qos", command.connect.will_qos);
            LogPrint("INFO", __FILE__, "%s: %u\n", "connect.will_retain", command.connect.will_retain);
            LogPrint("INFO", __FILE__, "%s: %s\n", "publish.message.topic.data", command.publish.message.topic.data);
            LogPrint("INFO", __FILE__, "%s: %u\n", "publish.message.topic.length", command.publish.message.topic.length);
            LogPrint("INFO", __FILE__, "%s: %s\n", "publish.message.payload.data", command.publish.message.payload.data);
            LogPrint("INFO", __FILE__, "%s: %u\n", "publish.message.payload.length", command.publish.message.payload.length);
            LogPrint("INFO", __FILE__, "%s: %u\n", "publish.qos", command.publish.qos);
            LogPrint("INFO", __FILE__, "%s: %u\n", "publish.retain", command.publish.retain);
            LogPrint("INFO", __FILE__, "%s: %s\n", "subscribe.topic.data", command.subscribe.topic.data);
            LogPrint("INFO", __FILE__, "%s: %u\n", "subscribe.topic.length", command.subscribe.topic.length);
            LogPrint("INFO", __FILE__, "%s: %u\n", "subscribe.qos", command.subscribe.qos);
            */

            // Process command
            switch (command.type)
            {
                case CONNECT:
                    MqttConnect(command.connect.will_message.topic.data,
                                command.connect.will_message.topic.length,
                                command.connect.will_message.payload.data,
                                command.connect.will_message.payload.length,
                                command.connect.will_qos,
                                command.connect.will_retain);
                    break;

                case PUBLISH:
                    MqttPublish(command.publish.message.topic.data,
                                command.publish.message.topic.length,
                                command.publish.message.payload.data,
                                command.publish.message.payload.length,
                                command.publish.qos,
                                command.publish.retain);
                    break;

                case SUBSCRIBE:
                    MqttSubscribe(command.subscribe.topic.data,
                                  command.subscribe.topic.length,
                                  command.subscribe.qos);
                    break;
            }
        }
    }
}

/*-----------------------------------------------------------*/

void MqttSubmitConnect(
    bool clean_session,
    uint16_t keep_alive,
    const char *client_id,
    uint16_t client_id_length,
    const char *username,
    uint16_t username_length,
    const char *password,
    uint16_t password_length,
    const char *will_topic,
    size_t will_topic_length,
    const char *will_payload,
    size_t will_payload_length,
    MQTTQoS_t will_qos,
    bool will_retain)
{
    if (client_id_length > MQTT_CLIENT_ID_BUFFER_SIZE)
    {
        LogPrint("FATAL", __FILE__, "client_id_length > MQTT_CLIENT_ID_BUFFER_SIZE\n");
        Fault();
    }

    if (username_length > MQTT_USERNAME_BUFFER_SIZE)
    {
        LogPrint("FATAL", __FILE__, "username_length > MQTT_USERNAME_BUFFER_SIZE\n");
        Fault();
    }

    if (password_length > MQTT_PASSWORD_BUFFER_SIZE)
    {
        LogPrint("FATAL", __FILE__, "password_length > MQTT_PASSWORD_BUFFER_SIZE\n");
        Fault();
    }

    if (will_topic_length > MQTT_TOPIC_BUFFER_SIZE)
    {
        LogPrint("FATAL", __FILE__, "will_topic_length > MQTT_TOPIC_BUFFER_SIZE\n");
        Fault();
    }

    if (will_payload_length > MQTT_PAYLOAD_BUFFER_SIZE)
    {
        LogPrint("FATAL", __FILE__, "will_payload_length > MQTT_PAYLOAD_BUFFER_SIZE\n");
        Fault();
    }

    MqttCommand_t command;
    memset(&command, 0, sizeof(MqttCommand_t));
    command.type = CONNECT;
    command.connect.clean_session = clean_session;
    command.connect.keep_alive = keep_alive;
    strncpy(command.connect.client_id, client_id, MQTT_CLIENT_ID_BUFFER_SIZE);
    command.connect.client_id_length = client_id_length;
    strncpy(command.connect.username, username, MQTT_USERNAME_BUFFER_SIZE);
    command.connect.username_length = username_length;
    strncpy(command.connect.password, password, MQTT_PASSWORD_BUFFER_SIZE);
    command.connect.password_length = password_length;
    strncpy(command.connect.will_message.topic.data, will_topic, MQTT_TOPIC_BUFFER_SIZE);
    command.connect.will_message.topic.length = will_topic_length;
    strncpy(command.connect.will_message.payload.data, will_payload, MQTT_PAYLOAD_BUFFER_SIZE);
    command.connect.will_message.payload.length = will_payload_length;
    command.connect.will_qos = will_qos;
    command.connect.will_retain = will_retain;

    if (xQueueSend(command_queue, &command, portMAX_DELAY) != pdTRUE)
    {
        LogPrint("FATAL", __FILE__, "Failed to send CONNECT to command_queue\n");
        Fault();
    }
}

/*-----------------------------------------------------------*/

void MqttSubmitPublish(const char *topic,
                       size_t topic_length,
                       const char *payload,
                       size_t payload_length,
                       MQTTQoS_t qos,
                       bool retain)
{
    LogPrint("INFO",
             __FILE__,
             "MqttSubmitPublish, t:'%s', tl:%u, p:'%s', pl:%u\n",
             topic,
             topic_length,
             payload,
             payload_length);

    if (topic_length > MQTT_TOPIC_BUFFER_SIZE)
    {
        LogPrint("FATAL", __FILE__, "topic_length > MQTT_TOPIC_BUFFER_SIZE\n");
        Fault();
    }

    if (payload_length > MQTT_PAYLOAD_BUFFER_SIZE)
    {
        LogPrint("FATAL", __FILE__, "payload_length > MQTT_PAYLOAD_BUFFER_SIZE\n");
        Fault();
    }

    MqttCommand_t command;
    memset(&command, 0, sizeof(MqttCommand_t));
    command.type = PUBLISH;
    strncpy(command.publish.message.topic.data, topic, MQTT_TOPIC_BUFFER_SIZE);
    command.publish.message.topic.length = topic_length;
    strncpy(command.publish.message.payload.data, payload, MQTT_PAYLOAD_BUFFER_SIZE);
    command.publish.message.payload.length = payload_length;
    command.publish.qos = qos;
    command.publish.retain = retain;
    LogPrint("INFO",
             __FILE__,
             "pre xQueueSend, t:'%s', tl:%u, p:'%s', pl:%u\n",
             command.publish.message.topic.data,
             command.publish.message.topic.length,
             command.publish.message.payload.data,
             command.publish.message.payload.length);

    if (xQueueSend(command_queue, &command, portMAX_DELAY) != pdTRUE)
    {
        LogPrint("FATAL", __FILE__, "Failed to send PUBLISH to command_queue\n");
        Fault();
    }
}

/*-----------------------------------------------------------*/

void MqttSubmitSubscribe(const char *topic,
                         size_t topic_length,
                         MQTTQoS_t qos)
{
    if (topic_length > MQTT_TOPIC_BUFFER_SIZE)
    {
        LogPrint("FATAL", __FILE__, "topic_length > MQTT_TOPIC_BUFFER_SIZE\n");
        Fault();
    }

    MqttCommand_t command;
    memset(&command, 0, sizeof(MqttCommand_t));
    command.type = SUBSCRIBE;
    strncpy(command.subscribe.topic.data, topic, MQTT_TOPIC_BUFFER_SIZE);
    command.subscribe.topic.length = topic_length;
    command.subscribe.qos = qos;

    if (xQueueSend(command_queue, &command, portMAX_DELAY) != pdTRUE)
    {
        LogPrint("FATAL", __FILE__, "Failed to send SUBSCRIBE to command_queue\n");
        Fault();
    }
}

/*-----------------------------------------------------------*/

MqttMessage_t MqttSubscriptionReceive()
{
    MqttMessage_t message;

    if (xQueueReceive(subscription_queue, &message, portMAX_DELAY) != pdTRUE)
    {
        LogPrint("FATAL", __FILE__, "subscription_queue receive failed\n");
        Fault();
    }

    return message;
}

/*-----------------------------------------------------------*/

static void MqttConnect(const char *will_topic,
                        size_t will_topic_length,
                        const char *will_payload,
                        size_t will_payload_length,
                        MQTTQoS_t will_qos,
                        bool will_retain)
{
    if (connection_state == CONNECTED)
    {
        LogPrint("FATAL", __FILE__, "MQTT already connected\n");
        Fault();
    }

    ActivityLedSetFlash(50);
    MqttTransportConnect();
    MqttContextInit();
    bool session_present;
    MQTTConnectInfo_t connect_info;
    memset(&connect_info, 0, sizeof(connect_info));
    connect_info.cleanSession = true;
    connect_info.keepAliveSeconds = MQTT_KEEP_ALIVE;
    connect_info.pClientIdentifier = MQTT_CLIENT_ID;
    connect_info.clientIdentifierLength = (uint16_t) strlen(MQTT_CLIENT_ID);
    connect_info.pUserName = MQTT_BROKER_USERNAME;
    connect_info.userNameLength = (uint16_t) strlen(MQTT_BROKER_USERNAME);
    connect_info.pPassword = MQTT_BROKER_PASSWORD;
    connect_info.passwordLength = (uint16_t) strlen(MQTT_BROKER_PASSWORD);
    // Publish to a topic
    MQTTPublishInfo_t will_info;
    memset(&will_info, 0, sizeof(will_info));
    will_info.qos = will_qos;
    will_info.retain = will_retain;
    will_info.pTopicName = will_topic;
    will_info.topicNameLength = (uint16_t) will_topic_length;
    will_info.pPayload = will_payload;
    will_info.payloadLength = (uint16_t) will_payload_length;
    LogPrint("INFO", __FILE__, "Attempting to connect to MQTT broker...\n");
    MQTTStatus_t status = MQTT_Connect(&mqtt_context, &connect_info, &will_info, 5000, &session_present);

    if (status != MQTTSuccess)
    {
        LogPrint("FATAL", __FILE__, "...MQTT broker connection failed\n");
        Fault();
    }

    LogPrint("INFO", __FILE__, "...MQTT broker connection success\n");
    connection_state = CONNECTED;
}

/*-----------------------------------------------------------*/

static void MqttContextInit()
{
    // Setup the main mqtt buffer
    network_buffer.pBuffer = packet_buffer;
    network_buffer.size = MQTT_PACKET_BUFFER_SIZE;
    LogPrint("INFO", __FILE__, "Attempting to initialise MQTT context...\n");
    MQTTStatus_t status = MQTT_Init(&mqtt_context, &transport_interface, GetTimeMs, MqttEventCallback, &network_buffer);

    if (status != MQTTSuccess)
    {
        LogPrint("FATAL", __FILE__, "...MQTT_Init initialise failed\n");
        Fault();
    }

    status = MQTT_InitStatefulQoS(&mqtt_context,
                                  outgoing_pub_record_buffer,
                                  MQTT_PUBLISH_LIST_SIZE,
                                  incoming_pub_record_buffer,
                                  MQTT_PUBLISH_LIST_SIZE);

    if (status != MQTTSuccess)
    {
        LogPrint("FATAL", __FILE__, "...MQTT_InitStatefulQoS initialise failed\n");
        Fault();
    }

    LogPrint("INFO", __FILE__, "...MQTT context initialise success\n");
}

/*-----------------------------------------------------------*/

void MqttEventCallback(MQTTContext_t *mqtt_context, MQTTPacketInfo_t *packet_info, MQTTDeserializedInfo_t *deserialized_info)
{
    if (deserialized_info->pPublishInfo != NULL)
    {
        LogPrint("DEBUG", __FILE__, "Received message on topic %.*s: %.*s\n",
                 deserialized_info->pPublishInfo->topicNameLength, deserialized_info->pPublishInfo->pTopicName,
                 deserialized_info->pPublishInfo->payloadLength, (const char *)deserialized_info->pPublishInfo->pPayload);

        if (deserialized_info->pPublishInfo->topicNameLength > MQTT_TOPIC_BUFFER_SIZE)
        {
            LogPrint("FATAL", __FILE__, "deserialized_info->pPublishInfo->topicNameLength > MQTT_TOPIC_BUFFER_SIZE\n");
            Fault();
        }

        if (deserialized_info->pPublishInfo->payloadLength > MQTT_PAYLOAD_BUFFER_SIZE)
        {
            LogPrint("FATAL", __FILE__, "deserialized_info->pPublishInfo->payloadLength > MQTT_PAYLOAD_BUFFER_SIZE\n");
            Fault();
        }

        MqttMessage_t message;
        strncpy(message.topic.data, deserialized_info->pPublishInfo->pTopicName, MQTT_TOPIC_BUFFER_SIZE);
        message.topic.length = deserialized_info->pPublishInfo->topicNameLength;
        strncpy(message.payload.data, deserialized_info->pPublishInfo->pPayload, MQTT_PAYLOAD_BUFFER_SIZE);
        message.payload.length = deserialized_info->pPublishInfo->payloadLength;

        if (xQueueSend(subscription_queue, &message, portMAX_DELAY) != pdTRUE)
        {
            LogPrint("FATAL", __FILE__, "Failed to send message to subscription_queue\n");
            Fault();
        }
    }
}

/*-----------------------------------------------------------*/

static void MqttTransportConnect()
{
    struct sockaddr_in server_address;
    int socket;
    LogPrint("DEBUG", __FILE__, "Creating lwip socket...\n");
    socket = lwip_socket(AF_INET, SOCK_STREAM, 0);

    if (socket < 0)
    {
        LogPrint("FATAL", __FILE__, "lwip_socket() failed\n");
        Fault();
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(MQTT_BROKER_PORT);
    server_address.sin_addr.s_addr = inet_addr(MQTT_BROKER_ADDRESS);
    LogPrint("DEBUG", __FILE__, "Connecting via lwip...\n");
    int lwip_connect_ret = lwip_connect(socket, (struct sockaddr *)&server_address, sizeof(server_address));

    if (lwip_connect_ret < 0)
    {
        LogPrint("FATAL", __FILE__, "lwip_connect failed with %i - is MQTT broker online?\n", lwip_connect_ret);
        lwip_close(socket);
        Fault();
    }

    // We don't want blocking sockets for coreMQTT
    int flags = lwip_fcntl(socket, F_GETFL, 0);
    lwip_fcntl(socket, F_SETFL, flags | O_NONBLOCK);
    // Store the socket descriptor in the network context
    network_context.socket = socket;
    // Set up the transport interface
    transport_interface.pNetworkContext = &network_context;
    transport_interface.send = (TransportSend_t) MqttTransportSend;
    transport_interface.recv = (TransportRecv_t) MqttTransportRecv;
}

/*-----------------------------------------------------------*/

static void MqttTransportDisconnect(NetworkContext_t *network_context)
{
    LogPrint("DEBUG", __FILE__, "MqttTransportDisconnect\n");
    lwip_close(network_context->socket);
}

/*-----------------------------------------------------------*/

static int32_t MqttTransportSend(NetworkContext_t *network_context, const void *buffer, size_t bytes_to_send)
{
    int32_t bytes_sent;
    int32_t result = lwip_send(network_context->socket, buffer, bytes_to_send, 0);

    // Can't send at the moment, but no error
    if (result == -1 && (errno == EWOULDBLOCK || errno == EAGAIN))
    {
        bytes_sent = 0;
    }
    // Send error
    else if (result < 0)
    {
        LogPrint("DEBUG", __FILE__, "Send failed: %s\n", result);
        return SEND_RECV_FAILED;
    }
    // Sent some some data
    else
    {
        char output_hex[(bytes_sent * 3) + 1];
        BytesToHex(output_hex, sizeof(output_hex), buffer, bytes_sent);
        LogPrint("DEBUG", __FILE__, "Sent bytes on socket: %s\n", &output_hex);
        bytes_sent = result;
    }

    return bytes_sent;
}

/*-----------------------------------------------------------*/

static int32_t MqttTransportRecv(NetworkContext_t *network_context, void *buffer, size_t bytes_to_recv)
{
    int32_t bytes_received;
    int32_t result = lwip_recv(network_context->socket, buffer, bytes_to_recv, 0);

    // Nothing to recv at the moment, but no error
    if (result == -1 && (errno == EWOULDBLOCK || errno == EAGAIN))
    {
        bytes_received = 0;
    }
    // Recv error
    else if (result < 0)
    {
        LogPrint("DEBUG", __FILE__, "Recv failed: %s\n", result);
        return SEND_RECV_FAILED;
    }
    // Got some data
    else
    {
        bytes_received = result;
        char output_hex[(bytes_received * 3) + 1];
        BytesToHex(output_hex, sizeof(output_hex), buffer, bytes_received);
        //LogPrint("DEBUG", __FILE__, "Recvd %i bytes on socket: %s\n", bytes_received, &output_hex);
    }

    return bytes_received;
}

/*-----------------------------------------------------------*/

static void MqttSubscribe(const char *topic,
                          size_t topic_length,
                          MQTTQoS_t qos)
{
    // Subscribe to a topic
    MQTTSubscribeInfo_t subscribe_info =
    {
        .pTopicFilter = topic,
        .topicFilterLength = (uint16_t) topic_length,
        .qos = qos
    };
    LogPrint("DEBUG",
             __FILE__,
             "Attempting to subscribe: t:'%s', tl:%u\n",
             subscribe_info.pTopicFilter,
             subscribe_info.topicFilterLength);
    uint16_t packet_id = MQTT_GetPacketId(&mqtt_context);
    MQTTStatus_t status = MQTT_Subscribe(&mqtt_context, &subscribe_info, 1, packet_id);

    if (status != MQTTSuccess)
    {
        LogPrint("FATAL", __FILE__, "...subscription failed with: %u\n", status);
        Fault();
    }

    LogPrint("INFO", __FILE__, "...subscription success\n");
}

/*-----------------------------------------------------------*/

static void MqttPublish(const char *topic,
                        size_t topic_length,
                        const char *payload,
                        size_t payload_length,
                        MQTTQoS_t qos,
                        bool retain)
{
    // Publish to a topic
    MQTTPublishInfo_t publish_info =
    {
        .qos = qos,
        .retain = retain,
        .pTopicName = topic,
        .topicNameLength = (uint16_t) topic_length,
        .pPayload = payload,
        .payloadLength = (uint16_t) payload_length
    };
    LogPrint("DEBUG",
             __FILE__,
             "Attempting to publish: t:'%s', tl:%u, p:'%s', pl:%u\n",
             publish_info.pTopicName,
             publish_info.topicNameLength,
             publish_info.pPayload,
             publish_info.payloadLength);
    uint16_t packet_id = MQTT_GetPacketId(&mqtt_context);
    MQTTStatus_t status = MQTT_Publish(&mqtt_context, &publish_info, packet_id);

    if (status != MQTTSuccess)
    {
        LogPrint("FATAL", __FILE__, "...publish failed with: %u\n", status);
        Fault();
    }

    LogPrint("INFO", __FILE__, "...publish success\n");
}
