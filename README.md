# Alert Panel

A FreeRTOS-Based App for the Raspberry Pi Pico W exposing the Pimoroni Pico RGB Keypad Base via MQTT over WiFI

## Prerequisites

1. Install: `sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib astyle`

## Build

1. Get the repo: `git clone https://github.com/tijy/alert-panel.git && cd alert-panel`
2. Get submodules: `git submodule update --init`
3. Get pico-sdk submodules: `cd lib/pico-sdk && git submodule update --init`
4. Configure credentials in: `include/alert_panel_config.h`
5. Prevent commiting credentials: `git update-index --assume-unchanged include/alert_panel_config.h`
6. Configure: `cd ../../build && cmake ..`
7. Make: `make alert-panel-app`
8. Upload to pico using BOOTSEL: `build/alert_panel_app.uf2`

## Styling

1. Use astyle: `astyle --options=./.astylerc ./src/*.c ./src/*.h ./include/*.h`

## Home Assistant

The MQTT messaging (payloads and topics) has been build to support easy integration into Home Assistant.
Each LED can be set up as a light entity and each Button can be configured to raise events.
See `example/alert_panel_1.yaml` for an example configuration of how to integrate into Home Assistant.