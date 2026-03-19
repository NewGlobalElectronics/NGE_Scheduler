cmake_minimum_required(VERSION 3.13)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(nge_scheduler_rp2040 C CXX ASM)
set(CMAKE_C_STANDARD 99)

pico_sdk_init()

add_executable(nge_scheduler_rp2040
    main.c
    bsp/bsp.c
    ../../core/NGE_Scheduler.c
    ../../core/app_tasks.c
)

target_include_directories(nge_scheduler_rp2040 PRIVATE
    ../../core
    bsp
)

target_link_libraries(nge_scheduler_rp2040
    pico_stdlib
    hardware_timer
    hardware_irq
)

pico_enable_stdio_usb(nge_scheduler_rp2040 1)
pico_enable_stdio_uart(nge_scheduler_rp2040 0)
pico_add_extra_outputs(nge_scheduler_rp2040)
