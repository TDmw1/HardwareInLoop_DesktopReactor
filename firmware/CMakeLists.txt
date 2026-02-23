# == LINES FOR VS CODE SETUP ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)

# 1. LOAD THE SDK (Must be before project command)
include(pico_sdk_import.cmake)

project(desktop_reactor C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# 2. INITIALIZE THE SDK
pico_sdk_init()

# 3. YOUR EXECUTABLE
add_executable(desktop_reactor
    main.c
)

# 4. LINK LIBRARIES
target_link_libraries(desktop_reactor 
    pico_stdlib
    hardware_pwm
    hardware_spi
    hardware_watchdog
)

# 6. ENABLE USB SERIAL
pico_enable_stdio_usb(desktop_reactor 1)
pico_enable_stdio_uart(desktop_reactor 0)

# 5. GENERATE OUTPUTS (.uf2 file)
pico_add_extra_outputs(desktop_reactor)
