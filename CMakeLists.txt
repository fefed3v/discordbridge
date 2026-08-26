cmake_minimum_required(VERSION 3.20)

project(
    DiscordBridge
    VERSION 0.0.7
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =========================================================
# PATHS
# =========================================================

set(SAMP_SDK_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/external/samp-plugin-sdk"
)

set(SRC_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# =========================================================
# VALIDATION
# =========================================================

if(NOT EXISTS "${SAMP_SDK_DIR}/plugincommon.h")
    message(FATAL_ERROR
        "SA-MP Plugin SDK nao encontrado em external/samp-plugin-sdk."
    )
endif()

if(NOT EXISTS "${SAMP_SDK_DIR}/amx/amx.h")
    message(FATAL_ERROR
        "AMX SDK nao encontrado em external/samp-plugin-sdk/amx."
    )
endif()

# =========================================================
# PLATFORM
# =========================================================

if(NOT WIN32)
    message(FATAL_ERROR
        "DiscordBridge v${PROJECT_VERSION} atualmente suporta apenas Windows."
    )
endif()

if(NOT MSVC)
    message(FATAL_ERROR
        "DiscordBridge deve ser compilado com MSVC."
    )
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR
        "DiscordBridge para SA-MP requer arquitetura Win32/x86. Configure o CMake usando -A Win32."
    )
endif()

# =========================================================
# SOURCES
# =========================================================

set(DISCORD_BRIDGE_SOURCES
    ${SRC_DIR}/core/BridgeCore.cpp

    ${SRC_DIR}/discord/DiscordClient.cpp
    ${SRC_DIR}/discord/Gateway.cpp
    ${SRC_DIR}/discord/HttpClient.cpp

    ${SRC_DIR}/discord/Embed.cpp
    ${SRC_DIR}/discord/Button.cpp
    ${SRC_DIR}/discord/SelectMenu.cpp
    ${SRC_DIR}/discord/Modal.cpp
    ${SRC_DIR}/discord/Command.cpp

    ${SRC_DIR}/discord/DiscordData.cpp
    ${SRC_DIR}/discord/ComponentsV2.cpp

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

add_library(
    DiscordBridge
    SHARED
    ${DISCORD_BRIDGE_SOURCES}
)

# =========================================================
# INCLUDE DIRECTORIES
# =========================================================

target_include_directories(
    DiscordBridge
    PRIVATE
        ${SRC_DIR}
        ${SAMP_SDK_DIR}
        ${SAMP_SDK_DIR}/amx
)

# =========================================================
# DEFINITIONS
# =========================================================

target_compile_definitions(
    DiscordBridge
    PRIVATE
        HAVE_STDINT_H
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _WINSOCK_DEPRECATED_NO_WARNINGS
        _CRT_SECURE_NO_WARNINGS
)

# =========================================================
# COMPILER OPTIONS
# =========================================================

target_compile_options(
    DiscordBridge
    PRIVATE
        /W4
        /permissive-
        /EHsc
        /utf-8
        /Zc:__cplusplus
        /wd4005
)

# =========================================================
# WINDOWS LIBRARIES
# =========================================================

target_link_libraries(
    DiscordBridge
    PRIVATE
        ws2_32
        winhttp
        crypt32
)

# =========================================================
# OUTPUT
# =========================================================

set_target_properties(
    DiscordBridge
    PROPERTIES
        OUTPUT_NAME "DiscordBridge"
        PREFIX ""
        SUFFIX ".dll"

        RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_BINARY_DIR}/DiscordBridge"

        ARCHIVE_OUTPUT_DIRECTORY
            "${CMAKE_BINARY_DIR}/DiscordBridge"

        LIBRARY_OUTPUT_DIRECTORY
            "${CMAKE_BINARY_DIR}/DiscordBridge"

        RUNTIME_OUTPUT_DIRECTORY_DEBUG
            "${CMAKE_BINARY_DIR}/DiscordBridge/Debug"

        RUNTIME_OUTPUT_DIRECTORY_RELEASE
            "${CMAKE_BINARY_DIR}/DiscordBridge/Release"

        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO
            "${CMAKE_BINARY_DIR}/DiscordBridge/RelWithDebInfo"

        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL
            "${CMAKE_BINARY_DIR}/DiscordBridge/MinSizeRel"

        ARCHIVE_OUTPUT_DIRECTORY_DEBUG
            "${CMAKE_BINARY_DIR}/DiscordBridge/Debug"

        ARCHIVE_OUTPUT_DIRECTORY_RELEASE
            "${CMAKE_BINARY_DIR}/DiscordBridge/Release"

        ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO
            "${CMAKE_BINARY_DIR}/DiscordBridge/RelWithDebInfo"

        ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL
            "${CMAKE_BINARY_DIR}/DiscordBridge/MinSizeRel"

        LIBRARY_OUTPUT_DIRECTORY_DEBUG
            "${CMAKE_BINARY_DIR}/DiscordBridge/Debug"

        LIBRARY_OUTPUT_DIRECTORY_RELEASE
            "${CMAKE_BINARY_DIR}/DiscordBridge/Release"

        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO
            "${CMAKE_BINARY_DIR}/DiscordBridge/RelWithDebInfo"

        LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL
            "${CMAKE_BINARY_DIR}/DiscordBridge/MinSizeRel"
)

# =========================================================
# BUILD INFORMATION
# =========================================================

message(STATUS "")
message(STATUS "==========================================")
message(STATUS " DiscordBridge ${PROJECT_VERSION}")
message(STATUS "==========================================")
message(STATUS "C++ Standard : C++${CMAKE_CXX_STANDARD}")
message(STATUS "Platform     : Windows")
message(STATUS "Compiler     : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Architecture : Win32/x86")
message(STATUS "Pointer Size : ${CMAKE_SIZEOF_VOID_P} bytes")
message(STATUS "Output       : DiscordBridge.dll")
message(STATUS "SA-MP SDK    : ${SAMP_SDK_DIR}")
message(STATUS "==========================================")
message(STATUS "")