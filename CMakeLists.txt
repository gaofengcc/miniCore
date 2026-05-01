cmake_minimum_required(VERSION 3.14)

project(miniCore LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

option(MINICORE_ENABLE_RPC "Build RPC support on top of broker" ON)
option(MINICORE_BUILD_EXAMPLES "Build miniCore examples" ON)

set(MINICORE_SOURCES
    src/osal/linux/epx_os_thread.c
    src/osal/linux/epx_os_mutex.c
    src/osal/linux/epx_os_queue.c
    src/osal/linux/epx_os_mem.c
    src/core/epx_broker.c
    src/core/epx_mempool.c
    src/core/epx_msg.c
    src/core/epx_topic.c
    src/core/epx_topic_tbl.c
)

if(MINICORE_ENABLE_RPC)
    list(APPEND MINICORE_SOURCES src/framework/epx_rpc.c)
endif()

add_library(minicore STATIC ${MINICORE_SOURCES})
add_library(miniCore::minicore ALIAS minicore)

target_include_directories(minicore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(minicore
    PUBLIC
        pthread
        rt
)

target_compile_definitions(minicore
    PUBLIC
        EPX_PLATFORM_LINUX=1
)

if(MINICORE_ENABLE_RPC)
    target_compile_definitions(minicore
        PUBLIC
            EPX_FRAMEWORK_ENABLE=1
            EPX_RPC_ENABLE=1
    )
endif()

if(MINICORE_BUILD_EXAMPLES)
    add_executable(minicore_pub_sub examples/minimal_pub_sub.c)
    target_link_libraries(minicore_pub_sub PRIVATE miniCore::minicore)

    if(MINICORE_ENABLE_RPC)
        add_executable(minicore_rpc examples/minimal_rpc.c)
        target_link_libraries(minicore_rpc PRIVATE miniCore::minicore)
    endif()
endif()
