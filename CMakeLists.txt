cmake_minimum_required(VERSION 3.20)

project(DiscordBridge VERSION 0.0.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =========================================================
# OPTIONS
# =========================================================

option(DBRIDGE_LINUX_32BIT "Build Linux plugin as x86/32-bit (required by classic SA-MP server)" ON)

# =========================================================
# PATHS
# =========================================================

set(SAMP_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/samp-plugin-sdk")
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(NOT EXISTS "${SAMP_SDK_DIR}/plugincommon.h")
    message(FATAL_ERROR "SA-MP Plugin SDK nao encontrado em external/samp-plugin-sdk.")
endif()

if(NOT EXISTS "${SAMP_SDK_DIR}/amx/amx.h")
    message(FATAL_ERROR "AMX SDK nao encontrado em external/samp-plugin-sdk/amx.")
endif()

# =========================================================
# COMMON SOURCES
# =========================================================

set(DISCORD_BRIDGE_SOURCES
    ${SRC_DIR}/core/BridgeCore.cpp

    ${SRC_DIR}/discord/DiscordClient.cpp
    ${SRC_DIR}/discord/Embed.cpp
    ${SRC_DIR}/discord/Button.cpp
    ${SRC_DIR}/discord/SelectMenu.cpp
    ${SRC_DIR}/discord/Modal.cpp

    ${SRC_DIR}/pawn/PawnRuntime.cpp
    ${SRC_DIR}/pawn/Natives.cpp

    ${SRC_DIR}/plugin/AmxExports.cpp
    ${SRC_DIR}/plugin/Plugin.cpp

    ${SAMP_SDK_DIR}/amxplugin.cpp
)

# =========================================================
# PLATFORM NETWORKING
# =========================================================

if(WIN32)
    # if(NOT MSVC)
    #     message(FATAL_ERROR "No Windows, use MSVC ou ClangCL.")
    # endif()

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "SA-MP Windows requer Win32/x86. Configure com -A Win32.")
    endif()

    list(APPEND DISCORD_BRIDGE_SOURCES
        ${SRC_DIR}/discord/Gateway.cpp
        ${SRC_DIR}/discord/HttpClient.cpp
        ${SRC_DIR}/plugin/DiscordBridge.def
    )
else()
    find_package(CURL 7.86 REQUIRED)
    find_package(Threads REQUIRED)

    list(APPEND DISCORD_BRIDGE_SOURCES
        ${SRC_DIR}/discord/GatewayLinux.cpp
        ${SRC_DIR}/discord/HttpClientLinux.cpp
    )
endif()

# =========================================================
# TARGET
# =========================================================

add_library(DiscordBridge SHARED ${DISCORD_BRIDGE_SOURCES})

target_include_directories(DiscordBridge PRIVATE
    ${SRC_DIR}
    ${SAMP_SDK_DIR}
    ${SAMP_SDK_DIR}/amx
)

target_compile_definitions(DiscordBridge PRIVATE HAVE_STDINT_H)

if(WIN32)
    target_compile_definitions(DiscordBridge PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _WINSOCK_DEPRECATED_NO_WARNINGS _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(DiscordBridge PRIVATE ws2_32 winhttp crypt32)
else()
    target_compile_definitions(DiscordBridge PRIVATE LINUX)
    target_link_libraries(DiscordBridge PRIVATE CURL::libcurl Threads::Threads)

    if(DBRIDGE_LINUX_32BIT)
        target_compile_options(DiscordBridge PRIVATE -m32)
        target_link_options(DiscordBridge PRIVATE -m32)
    endif()
endif()

# =========================================================
# COMPILER OPTIONS
# =========================================================

if(MSVC)
    target_compile_options(DiscordBridge PRIVATE /W4 /permissive- /EHsc /utf-8 /Zc:__cplusplus /wd4005)
else()
    target_compile_options(DiscordBridge PRIVATE -Wall -Wextra -Wpedantic)
endif()

# =========================================================
# OUTPUT
# =========================================================

set_target_properties(DiscordBridge PROPERTIES
    OUTPUT_NAME "DiscordBridge"
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/DiscordBridge"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/DiscordBridge"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/DiscordBridge"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/DiscordBridge/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/DiscordBridge/Release"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/DiscordBridge/Debug"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/DiscordBridge/Release"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/DiscordBridge/Debug"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/DiscordBridge/Release"
)

message(STATUS "DiscordBridge ${PROJECT_VERSION}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
if(WIN32)
    message(STATUS "Output: DiscordBridge.dll (Win32/x86)")
else()
    if(DBRIDGE_LINUX_32BIT)
        message(STATUS "Output: DiscordBridge.so (Linux x86/32-bit)")
    else()
        message(STATUS "Output: DiscordBridge.so (Linux native architecture)")
    endif()
endif()
message(STATUS "SA-MP SDK: ${SAMP_SDK_DIR}")
