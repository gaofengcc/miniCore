cmake_minimum_required(VERSION 3.14)

project(miniCore VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(MINICORE_ENABLE_RPC "Build RPC support on top of broker" ON)
option(MINICORE_BUILD_EXAMPLES "Build miniCore examples" ON)

# FREERTOS / RTTHREAD: parent project must supply RTOS include paths and link the BSP/kernel.
set(MINICORE_OSAL_PLATFORM "LINUX" CACHE STRING "OSAL backend: LINUX, FREERTOS, RTTHREAD")
set_property(CACHE MINICORE_OSAL_PLATFORM PROPERTY STRINGS LINUX FREERTOS RTTHREAD)
if(NOT MINICORE_OSAL_PLATFORM)
    set(MINICORE_OSAL_PLATFORM "LINUX" CACHE STRING "OSAL backend: LINUX, FREERTOS, RTTHREAD" FORCE)
endif()

option(MINICORE_BUILD_TESTS "Build CTest smoke tests (requires LINUX OSAL)" ON)

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
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(minicore PRIVATE -Wall -Wextra)
endif()

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
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(minicore_pub_sub PRIVATE -Wall -Wextra)
    endif()

    if(MINICORE_ENABLE_RPC)
        add_executable(minicore_rpc examples/minimal_rpc.c)
        target_link_libraries(minicore_rpc PRIVATE miniCore::minicore)
        if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(minicore_rpc PRIVATE -Wall -Wextra)
        endif()
    endif()
endif()

if(MINICORE_BUILD_TESTS AND MINICORE_OSAL_PLATFORM STREQUAL "LINUX")
    enable_testing()
    add_executable(minicore_broker_smoke tests/broker_smoke.c)
    target_link_libraries(minicore_broker_smoke PRIVATE miniCore::minicore)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(minicore_broker_smoke PRIVATE -Wall -Wextra)
    endif()
    add_test(NAME broker_smoke COMMAND minicore_broker_smoke)
elseif(MINICORE_BUILD_TESTS)
    message(WARNING "MINICORE_BUILD_TESTS is on but MINICORE_OSAL_PLATFORM is not LINUX; skipping tests.")
endif()

set(MINICORE_CMAKE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/minicore")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/minicoreConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/minicoreConfig.cmake"
    INSTALL_DESTINATION "${MINICORE_CMAKE_INSTALL_DIR}"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/minicoreConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(TARGETS minicore
    EXPORT minicore_export
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT minicore_export
    FILE minicoreTargets.cmake
    NAMESPACE miniCore::
    DESTINATION "${MINICORE_CMAKE_INSTALL_DIR}"
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/minicoreConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/minicoreConfigVersion.cmake"
    DESTINATION "${MINICORE_CMAKE_INSTALL_DIR}"
)
