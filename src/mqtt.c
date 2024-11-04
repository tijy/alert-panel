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
#define MQTT_BUFFER_SIZE    (1024)

/**
 * @brief
 *
 */
struct NetworkContext
{
    int socket_descriptor;
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
typedef void (* MqttSubscriptionCallback_t )( MqttMessage_t * message );

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
    union
    {
        MqttConnectData_t   connect;
        MqttPublishData_t   publish;
        MqttSubscribeData_t subscribe;
    } data;

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
static uint8_t mqtt_buffer[MQTT_BUFFER_SIZE];
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
static void MqttConnect(const char* will_topic,
                        size_t will_topic_length,
                        const char* will_payload,
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
 * @return uint32_t
 */
static uint32_t MqttGetTime(void);

/**
 * @brief
 *
 * @param mqtt_context
 * @param packet_info
 * @param deserialized_info
 */
static void MqttEventCallback(MQTTContext_t *mqtt_context,
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
static void MqttSubscribe(const char* topic,
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
static void MqttPublish(const char* topic,
                        size_t topic_length,
                        const char* payload,
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
    while (1)
    {
        MqttCommand_t command;

        // Wait for next command
        if (xQueueReceive(command_queue, &command, portMAX_DELAY) != pdTRUE)
        {
            LogPrint("FATAL", __FILE__, "command_queue receive failed\n");
            Fault();
        }

        // Process command
        switch (command.type)
        {
        case CONNECT:
            MqttConnect(command.data.connect.will_message.topic.data,
                        command.data.connect.will_message.topic.length,
                        command.data.connect.will_message.payload.data,
                        command.data.connect.will_message.payload.length,
                        command.data.connect.will_qos,
                        command.data.connect.will_retain);
        case PUBLISH:
            MqttPublish(command.data.publish.message.topic.data,
                        command.data.publish.message.topic.length,
                        command.data.publish.message.payload.data,
                        command.data.publish.message.payload.length,
                        command.data.publish.qos,
                        command.data.publish.retain);
        case SUBSCRIBE:
            MqttSubscribe(command.data.subscribe.topic.data,
                          command.data.subscribe.topic.length,
                          command.data.subscribe.qos);
        }
    }
}

/*-----------------------------------------------------------*/

void MqttSubmitConnect(
    bool clean_session,
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
    command.type = CONNECT;
    command.data.connect.clean_session = clean_session;
    command.data.connect.keep_alive = keep_alive;
    strncpy(command.data.connect.client_id, client_id, MQTT_CLIENT_ID_BUFFER_SIZE);
    command.data.connect.client_id_length = client_id_length;
    strncpy(command.data.connect.username, username, MQTT_USERNAME_BUFFER_SIZE);
    command.data.connect.username_length = username_length;
    strncpy(command.data.connect.password, password, MQTT_PASSWORD_BUFFER_SIZE);
    command.data.connect.password_length = password_length;
    strncpy(command.data.connect.will_message.topic.data, will_topic, MQTT_TOPIC_BUFFER_SIZE);
    command.data.connect.will_message.topic.length = will_topic_length;
    strncpy(command.data.connect.will_message.payload.data, will_payload, MQTT_PAYLOAD_BUFFER_SIZE);
    command.data.connect.will_message.payload.length = will_payload_length;
    command.data.connect.will_qos = will_qos;
    command.data.connect.will_retain = will_retain;

    if (xQueueSend(command_queue, &command, portMAX_DELAY) != pdTRUE)
    {
        LogPrint("FATAL", __FILE__, "Failed to send CONNECT to command_queue\n");
        Fault();
    }
}

/*-----------------------------------------------------------*/

void MqttSubmitPublish(const char * topic,
                       size_t topic_length,
                       const char * payload,
                       size_t payload_length,
                       MQTTQoS_t qos,
                       bool retain)
{
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
    command.type = PUBLISH;
    strncpy(command.data.publish.message.topic.data, topic, MQTT_TOPIC_BUFFER_SIZE);
    command.data.publish.message.topic.length = topic_length;
    strncpy(command.data.publish.message.payload.data, payload, MQTT_PAYLOAD_BUFFER_SIZE);
    command.data.publish.message.payload.length = payload_length;
    command.data.publish.qos = qos;
    command.data.publish.retain = retain;

    if (xQueueSend(command_queue, &command, portMAX_DELAY) != pdTRUE)
    {
        LogPrint("FATAL", __FILE__, "Failed to send PUBLISH to command_queue\n");
        Fault();
    }
}

/*-----------------------------------------------------------*/

void MqttSubmitSubscribe(const char * topic,
                         size_t topic_length,
                         MQTTQoS_t qos)
{
    if (topic_length > MQTT_TOPIC_BUFFER_SIZE)
    {
        LogPrint("FATAL", __FILE__, "topic_length > MQTT_TOPIC_BUFFER_SIZE\n");
        Fault();
    }

    MqttCommand_t command;
    command.type = SUBSCRIBE;
    strncpy(command.data.subscribe.topic.data, topic, MQTT_TOPIC_BUFFER_SIZE);
    command.data.subscribe.topic.length = topic_length;
    command.data.subscribe.qos = qos;

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

static void MqttConnect(const char* will_topic,
                        size_t will_topic_length,
                        const char* will_payload,
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
    // Setup the MQTT connect information
    network_buffer.pBuffer = mqtt_buffer;
    network_buffer.size = MQTT_BUFFER_SIZE;

    LogPrint("INFO", __FILE__, "Attempting to initialise MQTT context...\n");
    MQTTStatus_t status = MQTT_Init(&mqtt_context, &transport_interface, MqttGetTime, MqttEventCallback, &network_buffer);
    if (status != MQTTSuccess)
    {
        LogPrint("FATAL", __FILE__, "...MQTT context initialise failed\n");
        Fault();
    }

    LogPrint("INFO", __FILE__, "...MQTT context initialise success\n");
}

/*-----------------------------------------------------------*/

static uint32_t MqttGetTime(void)
{
    // Implement a platform-specific way to return current time in milliseconds.
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

/*-----------------------------------------------------------*/

static void MqttEventCallback(MQTTContext_t *mqtt_context, MQTTPacketInfo_t *packet_info, MQTTDeserializedInfo_t *deserialized_info)
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
    int socket_descriptor;

    LogPrint("DEBUG", __FILE__, "Creating lwip socket...\n");
    socket_descriptor = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (socket_descriptor < 0)
    {
        LogPrint("FATAL", __FILE__, "lwip_socket() failed\n");
        Fault();
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(MQTT_BROKER_PORT);
    server_address.sin_addr.s_addr = inet_addr(MQTT_BROKER_ADDRESS);

    LogPrint("DEBUG", __FILE__, "Connecting via lwip...\n");
    if (lwip_connect(socket_descriptor, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        LogPrint("FATAL", __FILE__, "lwip_connect() failed - is MQTT broker online?\n");
        lwip_close(socket_descriptor);
        Fault();
    }

    // Store the socket descriptor in the network context
    network_context.socket_descriptor = socket_descriptor;

    // Set up the transport interface
    transport_interface.pNetworkContext = &network_context;
    transport_interface.send = (TransportSend_t) MqttTransportSend;
    transport_interface.recv = (TransportRecv_t) MqttTransportRecv;
}

/*-----------------------------------------------------------*/

static void MqttTransportDisconnect(NetworkContext_t *network_context)
{
    LogPrint("DEBUG", __FILE__, "MqttTransportDisconnect\n");
    lwip_close(network_context->socket_descriptor);
}

/*-----------------------------------------------------------*/

static int32_t MqttTransportSend(NetworkContext_t *network_context, const void *buffer, size_t bytes_to_send)
{
    char output_hex[(bytes_to_send * 3) + 1];
    BytesToHex(output_hex, sizeof(output_hex), buffer, bytes_to_send);
    LogPrint("DEBUG", __FILE__, "MqttTransportSend: %s\n", &output_hex);

    int32_t bytes_sent = lwip_send(network_context->socket_descriptor, buffer, bytes_to_send, 0);
    return (bytes_sent >= 0) ? bytes_sent : SEND_RECV_FAILED;
}

/*-----------------------------------------------------------*/

static int32_t MqttTransportRecv(NetworkContext_t *network_context, void *buffer, size_t bytes_to_recv)
{
    int32_t bytes_received = lwip_recv(network_context->socket_descriptor, buffer, bytes_to_recv, 0);
    return (bytes_received >= 0) ? bytes_received : SEND_RECV_FAILED;

    char output_hex[(bytes_to_recv * 3) + 1];
    BytesToHex(output_hex, sizeof(output_hex), buffer, bytes_to_recv);
    LogPrint("DEBUG", __FILE__, "MqttTransportRecv: %s\n", &output_hex);
}

/*-----------------------------------------------------------*/

static void MqttSubscribe(const char* topic,
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

    LogPrint("INFO", __FILE__, "Attempting to subscribe to '%s'...\n", MQTT_TOPIC_SUBSCRIBE);
    uint16_t packet_id = MQTT_GetPacketId(&mqtt_context);
    MQTTStatus_t status = MQTT_Subscribe(&mqtt_context, &subscribe_info, 1, packet_id);
    if (status != MQTTSuccess)
    {
        LogPrint("FATAL", __FILE__, "...Subscription failed\n");
        Fault();
    }

    LogPrint("INFO", __FILE__, "...Subscription success\n");
}

/*-----------------------------------------------------------*/

static void MqttPublish(const char* topic,
                        size_t topic_length,
                        const char* payload,
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

    LogPrint("INFO", __FILE__, "Attempting to publish '%s' to '%s'...\n", publish_info.pPayload, publish_info.pTopicName);
    uint16_t packet_id = MQTT_GetPacketId(&mqtt_context);
    MQTTStatus_t status = MQTT_Publish(&mqtt_context, &publish_info, packet_id);
    if (status != MQTTSuccess)
    {
        LogPrint("FATAL", __FILE__, "...Publish failed\n");
        Fault();
    }

    LogPrint("INFO", __FILE__, "...Publish success\n");
}
