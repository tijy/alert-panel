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


#ifndef _ALERT_PANEL_CONFIG_H
#define _ALERT_PANEL_CONFIG_H

// Wifi connection parameters
#define WIFI_SSID              "xxx"
#define WIFI_PASSWORD          "xxx"

// Mqtt connection parameters
#define MQTT_BROKER_ADDRESS         "xxx" // Only works with IP addresses, DNS resolution not working at the moment
#define MQTT_BROKER_PORT            1883
#define MQTT_KEEP_ALIVE             10 // Keep alive second, don't set too large to ensure timely 'offline' will message delivery
#define MQTT_CLIENT_ID              "alert_panel_1"
#define MQTT_BROKER_USERNAME        "xxx"
#define MQTT_BROKER_PASSWORD        "xxx"
#define MQTT_CLIENT_ID_BUFFER_SIZE  30
#define MQTT_USERNAME_BUFFER_SIZE   30
#define MQTT_PASSWORD_BUFFER_SIZE   30

// Internal buffer sizes (Ensure these are all sized large enough for holding their respective data)
#define MQTT_PACKET_BUFFER_SIZE     50000 // Size of buffer for storing mqtt packet bytes during recv call
#define MQTT_TOPIC_BUFFER_SIZE      40   // Size of buffer storing topic data strings
#define MQTT_PAYLOAD_BUFFER_SIZE    200  // Size of buffer storing payload data strings
#define MQTT_PUBLISH_LIST_SIZE      200  // Maximum number of outstanding QoS 2 & 3 message

#endif //_ALERT_PANEL_CONFIG_H
