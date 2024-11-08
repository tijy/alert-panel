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
* @file led_msg.c
* @brief
*/
#include "led_msg.h"

// standard includes
#include <string.h>
#include <stdio.h>

// tiny-json includes
#include "tiny-json.h"

// alert-panel includes
#include "log.h"
#include "alert_panel_config.h"

// availability: from alert-panel to broker (will/publish)
#define AVAILABLE_TOPIC             MQTT_CLIENT_ID "/available"
#define AVAILABLE_PAYLOAD_ONLINE    "online"
#define AVAILABLE_PAYLOAD_OFFLINE   "offline"

// led commands: from broker to alert-panel (subscription)
#define LED_CMD_TOPIC               MQTT_CLIENT_ID "/led/cmd/#"

// led states: from alert-panel to broker (publish)
#define LED_STATE_TOPIC_FMT         MQTT_CLIENT_ID "/led/state/%c"

/**
 * @brief
 *
 */
const char LED_MSG_KEY_IDS[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

/**
 * @brief
 *
 */
static char json_str[MQTT_PAYLOAD_BUFFER_SIZE];

/**
 * @brief
 *
 */
static json_t pool[10]; // number of json attributes

/*-----------------------------------------------------------*/

void LedMsgBuildAvailableTopic(char *topic_buffer, size_t buffer_size)
{
    snprintf(topic_buffer, buffer_size, AVAILABLE_TOPIC);
}

/*-----------------------------------------------------------*/

void LedMsgBuildAvailablePayload(bool online, char *payload_buffer, size_t buffer_size)
{
    snprintf(payload_buffer, buffer_size, online ? AVAILABLE_PAYLOAD_ONLINE : AVAILABLE_PAYLOAD_OFFLINE);
}

/*-----------------------------------------------------------*/

void LedMsgBuildCmdTopic(char *topic_buffer, size_t buffer_size)
{
    snprintf(topic_buffer, buffer_size, LED_CMD_TOPIC);
}

/*-----------------------------------------------------------*/

bool LedMsgParseCmdTopic(KeypadLedParams_t *params, char *topic, size_t topic_length)
{
    if (topic_length <= 0)
    {
        LogPrint("ERROR", __FILE__, "Received led cmd message topic is 0 length\n");
        return false;
    }

    // Ensure this is topic meant for us
    int expected_topic_length = strlen(LED_CMD_TOPIC); // This includes the '#' char, which in this case will be 0-f, so the size is still valid

    if (topic_length != expected_topic_length)
    {
        LogPrint("ERROR", __FILE__, "Received led cmd message topic is unexpected length\n");
        return false;
    }

    if (strncmp(topic, LED_CMD_TOPIC, (topic_length - 1)) != 0)
    {
        LogPrint("ERROR", __FILE__, "Received led cmd message topic is not in expected form\n");
        return false;
    }

    params->key_id = topic[topic_length - 1]; // get the last character (key_id)
    LogPrint("DEBUG", __FILE__, "Received led cmd message for key: %c\n", params->key_id);
    return true;
}

/*-----------------------------------------------------------*/

bool LedMsgParseCmdPayload(KeypadLedParams_t *params, const char *payload, size_t payload_length)
{
    // Sanity check
    if (payload_length > MQTT_PAYLOAD_BUFFER_SIZE)
    {
        LogPrint("ERROR", __FILE__, "payload_length > MQTT_PAYLOAD_BUFFER_SIZE\n");
        return false;
    }

    // Safe to memcpy now, copy payload into json_string as this will be modified
    memcpy(json_str, payload, payload_length);

    if (payload_length == MQTT_PAYLOAD_BUFFER_SIZE)
    {
        json_str[MQTT_PAYLOAD_BUFFER_SIZE - 1] = '\0';
    }
    else
    {
        json_str[payload_length] = '\0';
    }

    // Create json object
    json_t const *json_obj = json_create(json_str, pool, sizeof(pool) / sizeof(pool[0]));

    if (!json_obj)
    {
        LogPrint("ERROR", __FILE__, "Error parsing received mqtt message to json\n");
        return false;
    }

    // Parse brightness
    json_t const *brightness_prop = json_getProperty(json_obj, "brightness");

    if (brightness_prop && json_getType(brightness_prop) == JSON_INTEGER)
    {
        params->brightness = (char)json_getInteger(brightness_prop);
        params->brightness_set = true;
    }

    // Parse color
    json_t const *color_prop = json_getProperty(json_obj, "color");

    if (color_prop && json_getType(color_prop) == JSON_OBJ)
    {
        json_t const *r_prop = json_getProperty(color_prop, "r");
        json_t const *g_prop = json_getProperty(color_prop, "g");
        json_t const *b_prop = json_getProperty(color_prop, "b");

        if (r_prop && g_prop && b_prop &&
                json_getType(r_prop) == JSON_INTEGER &&
                json_getType(g_prop) == JSON_INTEGER &&
                json_getType(b_prop) == JSON_INTEGER)
        {
            params->red = (char)json_getInteger(r_prop);
            params->green = (char)json_getInteger(g_prop);
            params->blue = (char)json_getInteger(b_prop);
            params->colour_set = true;
        }
    }

    // Parse effect
    json_t const *effect_prop = json_getProperty(json_obj, "effect");

    if (effect_prop && json_getType(effect_prop) == JSON_TEXT)
    {
        const char *effect_str = json_getValue(effect_prop);

        if (strcmp(effect_str, "none") == 0)
        {
            params->effect = NONE;
            params->effect_set = true;
        }
        else if (strcmp(effect_str, "flash") == 0)
        {
            params->effect = FLASH;
            params->effect_set = true;
        }
        else if (strcmp(effect_str, "pulse") == 0)
        {
            params->effect = PULSE;
            params->effect_set = true;
        }
    }

    // Parse state
    json_t const *state_prop = json_getProperty(json_obj, "state");

    if (state_prop && json_getType(state_prop) == JSON_TEXT)
    {
        const char *state_str = json_getValue(state_prop);

        if (strcmp(state_str, "ON") == 0)
        {
            params->state = true;
            params->state_set = true;
        }
        else if (strcmp(state_str, "OFF") == 0)
        {
            params->state = false;
            params->state_set = true;
        }
    }

    return true;
}

/*-----------------------------------------------------------*/

void LedMsgBuildStateTopic(KeypadLedParams_t *params, char *topic_buffer, size_t buffer_size)
{
    snprintf(topic_buffer,
             buffer_size,
             LED_STATE_TOPIC_FMT,
             params->key_id);
}

/*-----------------------------------------------------------*/

void LedMsgBuildStatePayload(KeypadLedParams_t *params, char *payload_buffer, size_t buffer_size)
{
    // 1) Start json
    snprintf(payload_buffer, buffer_size, "{");
    bool comma_needed = false;

    // 2) Add state if state_set
    if (params->state_set)
    {
        snprintf(payload_buffer + strlen(payload_buffer), buffer_size - strlen(payload_buffer),
                 "\"state\": \"%s\"", params->state ? "ON" : "OFF");
        comma_needed = true;
    }

    // 3) Add brightness if brightness_set
    if (params->brightness_set)
    {
        if (comma_needed)
        {
            strncat(payload_buffer, ", ", buffer_size - strlen(payload_buffer) - 1);
        }

        snprintf(payload_buffer + strlen(payload_buffer), buffer_size - strlen(payload_buffer),
                 "\"brightness\": %d", params->brightness);
        comma_needed = true;
    }

    // 4) Add color field if colour_set
    if (params->colour_set)
    {
        if (comma_needed)
        {
            strncat(payload_buffer, ", ", buffer_size - strlen(payload_buffer) - 1);
        }

        snprintf(payload_buffer + strlen(payload_buffer), buffer_size - strlen(payload_buffer),
                 "\"color\": {\"r\": %d, \"g\": %d, \"b\": %d}",
                 params->red, params->green, params->blue);
        comma_needed = true;
    }

    // 5) Add color_mode (required for home assistant) if colour_set
    if (params->colour_set)
    {
        if (comma_needed)
        {
            strncat(payload_buffer, ", ", buffer_size - strlen(payload_buffer) - 1);
        }

        strncat(payload_buffer, "\"color_mode\": \"rgb\"", buffer_size - strlen(payload_buffer) - 1);
    }

    // 6) End json
    strncat(payload_buffer, "}", buffer_size - strlen(payload_buffer) - 1);
}
