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
* @file wifi.c
* @brief
*/
#include "wifi.h"

// pico-sdk includes
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

// alert-panel includes
#include "activity_led.h"
#include "log.h"
#include "system.h"

/*-----------------------------------------------------------*/

void WifiInit()
{
    if (cyw43_arch_init())
    {
        LogPrint("FATAL", __FILE__, "Failed to initialise cyw43 chip\n");
        Fault();
    }
}

/*-----------------------------------------------------------*/

void WifiConnect(const char * ssid, const char * password)
{
    ActivityLedSetFlash(25);

    cyw43_arch_enable_sta_mode();

    LogPrint("INFO", __FILE__, "Attempting to connecting to '%s' WiFi...\n", ssid);

    if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        LogPrint("FATAL", __FILE__, "...WiFi connection failed\n");
        Fault();
    }

    LogPrint("INFO", __FILE__, "...WiFi connection success\n");
}