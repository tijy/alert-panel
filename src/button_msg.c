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
* @file button_msg.c
* @brief
*/
#include "button_msg.h"

// standard includes
#include <string.h>
#include <stdio.h>

// tiny-json includes
#include "tiny-json.h"

// alert-panel includes
#include "log.h"
#include "alert_panel_config.h"

// button states: from alert-panel to broker (publish)
#define BUTTON_STATE_TOPIC_FMT         MQTT_CLIENT_ID "/button/state/%c"
#define BUTTON_STATE_PAYLOAD_PRESS    "press"
#define BUTTON_STATE_PAYLOAD_HOLD     "hold"

/*-----------------------------------------------------------*/

void ButtonMsgBuildStateTopic(KeypadButtonParams_t *params, char *topic_buffer, size_t buffer_size)
{
    snprintf(topic_buffer,
             buffer_size,
             BUTTON_STATE_TOPIC_FMT,
             params->key_id);
}

/*-----------------------------------------------------------*/

void ButtonMsgBuildStatePayload(KeypadButtonParams_t *params, char *payload_buffer, size_t buffer_size)
{
    // 1) Start json
    snprintf(payload_buffer, buffer_size, "{");
    bool comma_needed = false;
    // 2) Add event
    snprintf(payload_buffer + strlen(payload_buffer), buffer_size - strlen(payload_buffer),
             "\"event_type\":\"%s\"", params->event == PRESS ? BUTTON_STATE_PAYLOAD_PRESS : BUTTON_STATE_PAYLOAD_HOLD);
    comma_needed = true;
    // 3) End json
    strncat(payload_buffer, "}", buffer_size - strlen(payload_buffer) - 1);
}
