cmake_minimum_required(VERSION 3.15)
project(NGE_Scheduler C)
set(CMAKE_C_STANDARD 99)

# ── Compiler warnings ────────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ── Core library (shared by every port) ─────────────────────────────────────
add_library(nge_core STATIC
    core/NGE_Scheduler.c
    core/app_tasks.c
)
target_include_directories(nge_core PUBLIC core)

# ── Host ports ───────────────────────────────────────────────────────────────
if(UNIX AND NOT APPLE)
    add_executable(nge_linux
        ports/x86_linux/main.c
        ports/x86_linux/bsp/bsp.c
    )
    target_include_directories(nge_linux PRIVATE
        core
        ports/x86_linux/bsp
    )
    target_link_libraries(nge_linux PRIVATE nge_core)
endif()

if(WIN32)
    add_executable(nge_windows
        ports/x86_windows/main.c
        ports/x86_windows/bsp/bsp.c
    )
    target_include_directories(nge_windows PRIVATE
        core
        ports/x86_windows/bsp
    )
    target_link_libraries(nge_windows PRIVATE nge_core winmm)
endif()

# Embedded ports are built by their own toolchain files / IDE projects.
# See each ports/<target>/CMakeLists.txt or the README for instructions.
