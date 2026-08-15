cmake_minimum_required(VERSION 3.20)

project(DiscordBridge VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =========================================================
# PLATFORM
# =========================================================

if(NOT WIN32)
    message(FATAL_ERROR "DiscordBridge atualmente suporta apenas Windows.")
endif()

if(NOT MSVC)
    message(FATAL_ERROR "DiscordBridge atualmente requer MSVC ou ClangCL.")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "DiscordBridge deve ser compilado em Win32/x86. Use -A Win32.")
endif()

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
# SOURCES
# =========================================================

set(DISCORD_BRIDGE_SOURCES
    ${SRC_DIR}/core/BridgeCore.cpp

    ${SRC_DIR}/discord/DiscordClient.cpp
    ${SRC_DIR}/discord/Embed.cpp
    ${SRC_DIR}/discord/Gateway.cpp
    ${SRC_DIR}/discord/HttpClient.cpp

    ${SRC_DIR}/pawn/PawnRuntime.cpp
    ${SRC_DIR}/pawn/Natives.cpp

    ${SRC_DIR}/plugin/AmxExports.cpp
    ${SRC_DIR}/plugin/Plugin.cpp
    ${SRC_DIR}/plugin/DiscordBridge.def

    ${SAMP_SDK_DIR}/amxplugin.cpp
)

# =========================================================
# TARGET
# =========================================================

add_library(DiscordBridge SHARED ${DISCORD_BRIDGE_SOURCES})

# =========================================================
# INCLUDES
# =========================================================

target_include_directories(DiscordBridge PRIVATE
    ${SRC_DIR}
    ${SAMP_SDK_DIR}
    ${SAMP_SDK_DIR}/amx
)

# =========================================================
# DEFINITIONS
# =========================================================

target_compile_definitions(DiscordBridge PRIVATE
    HAVE_STDINT_H
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _WINSOCK_DEPRECATED_NO_WARNINGS
    _CRT_SECURE_NO_WARNINGS
)

# =========================================================
# WINDOWS LIBRARIES
# =========================================================

target_link_libraries(DiscordBridge PRIVATE
    ws2_32
    winhttp
    crypt32
)

# =========================================================
# COMPILER OPTIONS
# =========================================================

if(MSVC)
    target_compile_options(DiscordBridge PRIVATE
        /W4
        /permissive-
        /EHsc
        /wd4005
    )
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

# =========================================================
# BUILD INFO
# =========================================================

message(STATUS "DiscordBridge ${PROJECT_VERSION}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Architecture: Win32/x86")
message(STATUS "SA-MP SDK: ${SAMP_SDK_DIR}")