add_library(coreMQTT STATIC)
include("${CMAKE_SOURCE_DIR}/lib/coreMQTT/mqttFilePaths.cmake")

target_sources(coreMQTT PUBLIC
	${MQTT_SOURCES}
	${MQTT_SERIALIZER_SOURCES}
)

target_include_directories(coreMQTT PUBLIC 
	${MQTT_INCLUDE_PUBLIC_DIRS}
	${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(coreMQTT FreeRTOS-Kernel-Heap4 pico_stdlib)