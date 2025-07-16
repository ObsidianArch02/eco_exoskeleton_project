cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_exoskeleton_firmware)

# Set the target to esp32
set(CMAKE_EXECUTABLE_SUFFIX ".elf")
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

# Add shared components
add_library(shared_components STATIC
    mqtt_helper.cpp
    debug_helper.cpp
)

target_include_directories(shared_components PUBLIC .)

# Bubble machine module
add_executable(bubble_machine_module
    bubble_machine_module.c
)

target_link_libraries(bubble_machine_module shared_components)

# Greenhouse module
add_executable(greenhouse_module
    greenhouse_module.c
)

target_link_libraries(greenhouse_module shared_components)

# Injection module
add_executable(injection_module
    injection_module.c
)

target_link_libraries(injection_module shared_components)

# ESP-IDF component requirements
set(COMPONENT_REQUIRES 
    freertos
    esp_netif
    esp_event
    nvs_flash
    esp_wifi
    esp_eth
    tcp_transport
    esp_http_client
    esp_https_ota
    esp_https_server
    esp-tls
    mbedtls
    json
    cJSON
    driver
    log
)

# Include ESP-IDF components
target_link_libraries(bubble_machine_module ${COMPONENT_REQUIRES})
target_link_libraries(greenhouse_module ${COMPONENT_REQUIRES})
target_link_libraries(injection_module ${COMPONENT_REQUIRES})

# Set compiler options
target_compile_options(shared_components PRIVATE -Wno-unused-variable -Wno-unused-function)
target_compile_options(bubble_machine_module PRIVATE -Wno-unused-variable)
target_compile_options(greenhouse_module PRIVATE -Wno-unused-variable)
target_compile_options(injection_module PRIVATE -Wno-unused-variable)

# Add flash and monitor targets
add_custom_target(flash_bubble
    COMMAND idf.py -p ${SERIAL_PORT} flash -e bubble_machine_module
    DEPENDS bubble_machine_module
    COMMENT "Flashing bubble machine module"
)

add_custom_target(flash_greenhouse
    COMMAND idf.py -p ${SERIAL_PORT} flash -e greenhouse_module
    DEPENDS greenhouse_module
    COMMENT "Flashing greenhouse module"
)

add_custom_target(flash_injection
    COMMAND idf.py -p ${SERIAL_PORT} flash -e injection_module
    DEPENDS injection_module
    COMMENT "Flashing injection module"
)

add_custom_target(monitor
    COMMAND idf.py -p ${SERIAL_PORT} monitor
    COMMENT "Starting serial monitor"
)