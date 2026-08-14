cmake_minimum_required(VERSION 3.20)

project(DiscordBridge VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "DiscordBridge currently targets Windows.")
endif()

if(NOT MSVC)
    message(FATAL_ERROR "DiscordBridge currently requires MSVC.")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "DiscordBridge must be built as Win32/x86. Use -A Win32.")
endif()

# =========================================================
# PATHS
# =========================================================

set(DPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/dpp")
set(SAMP_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/samp-plugin-sdk")

if(NOT EXISTS "${DPP_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "DPP not found.")
endif()

if(NOT EXISTS "${SAMP_SDK_DIR}/plugincommon.h")
    message(FATAL_ERROR "SA-MP Plugin SDK not found.")
endif()

# =========================================================
# DPP
# =========================================================

set(DPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/dpp")
set(SAMP_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/samp-plugin-sdk")

set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
set(BUILD_VOICE_SUPPORT OFF CACHE BOOL "Disable DPP voice support" FORCE)
set(DPP_BUILD_TEST OFF CACHE BOOL "Disable DPP tests" FORCE)
set(DPP_INSTALL OFF CACHE BOOL "Disable DPP install target" FORCE)
set(DPP_NO_CORO ON CACHE BOOL "Disable DPP coroutines" FORCE)
set(DPP_NO_VCPKG ON CACHE BOOL "Disable VCPKG" FORCE)
set(WINDOWS_32_BIT ON CACHE BOOL "Build DPP for 32-bit Windows" FORCE)

add_subdirectory("${DPP_DIR}" "${CMAKE_BINARY_DIR}/external/dpp" EXCLUDE_FROM_ALL)

target_include_directories(dpp PRIVATE
    "${DPP_DIR}/win32/include"
)

target_compile_definitions(dpp PRIVATE
    OPENSSL_SYS_WIN32
    _WINSOCK_DEPRECATED_NO_WARNINGS
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_DEPRECATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# =========================================================
# DISCORD BRIDGE
# =========================================================

add_library(DiscordBridge SHARED
    src/core/BridgeCore.cpp
    src/discord/DiscordClient.cpp

    src/pawn/PawnRuntime.cpp
    src/pawn/natives/VersionNatives.cpp
    src/pawn/natives/BasicNatives.cpp

    src/plugin/Plugin.cpp
    src/plugin/AmxExports.cpp
    src/plugin/DiscordBridge.def

    "${SAMP_SDK_DIR}/amxplugin.cpp"
)

target_include_directories(DiscordBridge PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${SAMP_SDK_DIR}"
    "${SAMP_SDK_DIR}/amx"
)

target_compile_definitions(DiscordBridge PRIVATE
    HAVE_STDINT_H
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_link_libraries(DiscordBridge PRIVATE
    dpp
)

set_target_properties(DiscordBridge PROPERTIES
    OUTPUT_NAME "DiscordBridge"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/DiscordBridge/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/DiscordBridge/Debug"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release"
)

# =========================================================
# TEMPORARY RUNTIME DEPENDENCIES
# =========================================================

set(DPP_RUNTIME_DIR "${DPP_DIR}/win32/32/bin")

add_custom_command(TARGET DiscordBridge POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:dpp>" "$<TARGET_FILE_DIR:DiscordBridge>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DPP_RUNTIME_DIR}/libssl-1_1.dll" "$<TARGET_FILE_DIR:DiscordBridge>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DPP_RUNTIME_DIR}/libcrypto-1_1.dll" "$<TARGET_FILE_DIR:DiscordBridge>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DPP_RUNTIME_DIR}/zlib1.dll" "$<TARGET_FILE_DIR:DiscordBridge>"
)