cmake_minimum_required(VERSION 3.27)

# set pico board type for wifi support
set(PICO_BOARD pico_w)

# import pico-sdk
include(lib/pico-sdk/pico_sdk_init.cmake)

# import FreeRTOS-Kernel (RP2040)
include(lib/FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

# import coreMQTT
include(coreMQTT_import.cmake)

# import tiny-json
include(tiny-json_import.cmake)

# project
project(alert_panel)

# initialize the pico-sdk
pico_sdk_init()

# Define executable
add_executable(alert_panel_app
    src/activity_led.c
    src/keypad_driver.c
    src/log.c
    src/mqtt.c
    src/system.c
    src/util.c
    src/main.c
    src/wifi.c
)

# include directories
target_include_directories(alert_panel_app PUBLIC
    ${CMAKE_SOURCE_DIR}/include  
    ${CMAKE_SOURCE_DIR}/src)

# link libraries
target_link_libraries(alert_panel_app 
                        pico_stdlib 
                        pico_cyw43_arch_lwip_sys_freertos
                        hardware_i2c
                        hardware_spi
                        coreMQTT
                        tiny-json
                        pico_lwip_iperf                  
                        FreeRTOS-Kernel-Heap4)

# enable usb output, disable uart output
pico_enable_stdio_usb(alert_panel_app 1)
pico_enable_stdio_uart(alert_panel_app 0)

# create uf2 file
pico_add_extra_outputs(alert_panel_app)
