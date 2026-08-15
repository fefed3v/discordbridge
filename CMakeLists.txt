cmake_minimum_required(VERSION 3.20)

project(DiscordBridge VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "DiscordBridge atualmente suporta apenas Windows.")
endif()

if(NOT MSVC)
    message(FATAL_ERROR "DiscordBridge atualmente requer MSVC.")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "DiscordBridge deve ser compilado em Win32/x86. Use -A Win32.")
endif()

# =========================================================
# PATHS
# =========================================================

set(SAMP_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/samp-plugin-sdk")

if(NOT EXISTS "${SAMP_SDK_DIR}/plugincommon.h")
    message(FATAL_ERROR "SA-MP Plugin SDK nao encontrado em external/samp-plugin-sdk.")
endif()

# =========================================================
# DISCORD BRIDGE
# =========================================================

add_library(DiscordBridge SHARED
    src/core/BridgeCore.cpp

    src/discord/DiscordClient.cpp
    src/discord/http/HttpClient.cpp
    src/discord/gateway/GatewayInfo.cpp
    src/discord/gateway/GatewayClient.cpp
    src/discord/events/DiscordEvents.cpp

    src/pawn/PawnRuntime.cpp
    src/pawn/natives/BasicNatives.cpp
    src/pawn/natives/VersionNatives.cpp

    src/plugin/Plugin.cpp
    src/plugin/AmxExports.cpp
    src/plugin/DiscordBridge.def

    external/samp-plugin-sdk/amxplugin.cpp
)

# =========================================================
# INCLUDES
# =========================================================

target_include_directories(DiscordBridge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
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
# OUTPUT
# =========================================================

set_target_properties(DiscordBridge PROPERTIES
    OUTPUT_NAME "DiscordBridge"

    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/DiscordBridge/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/DiscordBridge/Release"

    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/DiscordBridge/Debug"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/DiscordBridge/Release"
)