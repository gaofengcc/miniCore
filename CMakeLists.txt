cmake_minimum_required(VERSION 3.14)

project(miniCore LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

option(MINICORE_ENABLE_RPC "Build RPC support on top of broker" ON)
option(MINICORE_BUILD_EXAMPLES "Build miniCore examples" ON)

# FREERTOS / RTTHREAD: parent project must supply RTOS include paths and link the BSP/kernel.
set(MINICORE_OSAL_PLATFORM "LINUX" CACHE STRING "OSAL backend: LINUX, FREERTOS, RTTHREAD")
set_property(CACHE MINICORE_OSAL_PLATFORM PROPERTY STRINGS LINUX FREERTOS RTTHREAD)
if(NOT MINICORE_OSAL_PLATFORM)
    set(MINICORE_OSAL_PLATFORM "LINUX" CACHE STRING "OSAL backend: LINUX, FREERTOS, RTTHREAD" FORCE)
endif()

set(MINICORE_SOURCES
    src/core/epx_broker.c
    src/core/epx_mempool.c
    src/core/epx_msg.c
    src/core/epx_topic.c
    src/core/epx_topic_tbl.c
)

if(NOT MINICORE_OSAL_PLATFORM MATCHES "^(LINUX|FREERTOS|RTTHREAD)$")
    message(FATAL_ERROR "MINICORE_OSAL_PLATFORM must be LINUX, FREERTOS, or RTTHREAD")
endif()

if(MINICORE_OSAL_PLATFORM STREQUAL "LINUX")
    list(APPEND MINICORE_SOURCES
        src/osal/linux/epx_os_thread.c
        src/osal/linux/epx_os_mutex.c
        src/osal/linux/epx_os_queue.c
        src/osal/linux/epx_os_mem.c
    )
elseif(MINICORE_OSAL_PLATFORM STREQUAL "FREERTOS")
    list(APPEND MINICORE_SOURCES
        src/osal/freertos/epx_os_thread.c
        src/osal/freertos/epx_os_mutex.c
        src/osal/freertos/epx_os_queue.c
        src/osal/freertos/epx_os_mem.c
    )
elseif(MINICORE_OSAL_PLATFORM STREQUAL "RTTHREAD")
    list(APPEND MINICORE_SOURCES
        src/osal/rtthread/epx_os_thread.c
        src/osal/rtthread/epx_os_mutex.c
        src/osal/rtthread/epx_os_queue.c
        src/osal/rtthread/epx_os_mem.c
    )
endif()

if(MINICORE_ENABLE_RPC)
    list(APPEND MINICORE_SOURCES src/framework/epx_rpc.c)
endif()

add_library(minicore STATIC ${MINICORE_SOURCES})
add_library(miniCore::minicore ALIAS minicore)

target_include_directories(minicore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MINICORE_OSAL_PLATFORM STREQUAL "LINUX")
    target_link_libraries(minicore
        PUBLIC
            pthread
            rt
    )
    target_compile_definitions(minicore
        PUBLIC
            EPX_PLATFORM_LINUX=1
    )
elseif(MINICORE_OSAL_PLATFORM STREQUAL "FREERTOS")
    target_compile_definitions(minicore
        PUBLIC
            EPX_PLATFORM_FREERTOS=1
    )
elseif(MINICORE_OSAL_PLATFORM STREQUAL "RTTHREAD")
    target_compile_definitions(minicore
        PUBLIC
            EPX_PLATFORM_RTTHREAD=1
    )
endif()

if(MINICORE_ENABLE_RPC)
    target_compile_definitions(minicore
        PUBLIC
            EPX_FRAMEWORK_ENABLE=1
            EPX_RPC_ENABLE=1
    )
endif()

if(MINICORE_BUILD_EXAMPLES AND MINICORE_OSAL_PLATFORM STREQUAL "LINUX")
    add_executable(minicore_pub_sub examples/minimal_pub_sub.c)
    target_link_libraries(minicore_pub_sub PRIVATE miniCore::minicore)

    if(MINICORE_ENABLE_RPC)
        add_executable(minicore_rpc examples/minimal_rpc.c)
        target_link_libraries(minicore_rpc PRIVATE miniCore::minicore)
    endif()
endif()
