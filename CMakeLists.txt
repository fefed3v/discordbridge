cmake_minimum_required(VERSION 3.20)

project(DiscordBridge VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "DiscordBridge Patch 0.0.1 currently targets Windows only.")
endif()

if(NOT MSVC)
    message(FATAL_ERROR "DiscordBridge Patch 0.0.1 currently requires MSVC.")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "DiscordBridge must be built as Win32/x86. Configure CMake with -A Win32.")
endif()

set(DISCORD_BRIDGE_DPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/dpp")
set(DISCORD_BRIDGE_OPENMP_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/open.mp-sdk")
set(DISCORD_BRIDGE_OPENMP_PAWN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/open.mp-pawn")
set(DISCORD_BRIDGE_SAMP_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/samp-plugin-sdk")

foreach(REQUIRED_FILE
    "${DISCORD_BRIDGE_DPP_DIR}/CMakeLists.txt"
    "${DISCORD_BRIDGE_OPENMP_SDK_DIR}/include/sdk.hpp"
    "${DISCORD_BRIDGE_OPENMP_PAWN_DIR}/source/amx/amx.h"
    "${DISCORD_BRIDGE_SAMP_SDK_DIR}/plugincommon.h"
    "${DISCORD_BRIDGE_SAMP_SDK_DIR}/amxplugin.cpp"
)
    if(NOT EXISTS "${REQUIRED_FILE}")
        message(FATAL_ERROR "Required dependency file not found: ${REQUIRED_FILE}. Make sure the external dependencies/submodules are present.")
    endif()
endforeach()

# =========================================================
# DPP
# =========================================================

# We intentionally use the x86 Windows dependencies bundled by this DPP checkout.
# DPP's current Win32/32 CMake branch links the correct x86 .lib files, but does
# not add the bundled include directory/Windows definitions itself, so we add
# those usage requirements immediately after creating the dpp target.
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(BUILD_VOICE_SUPPORT OFF CACHE BOOL "" FORCE)
set(DPP_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(DPP_INSTALL OFF CACHE BOOL "" FORCE)
set(DPP_NO_CORO ON CACHE BOOL "" FORCE)
set(DPP_NO_VCPKG ON CACHE BOOL "" FORCE)
set(WINDOWS_32_BIT ON CACHE BOOL "" FORCE)

add_subdirectory("${DISCORD_BRIDGE_DPP_DIR}" "${CMAKE_BINARY_DIR}/external/dpp" EXCLUDE_FROM_ALL)

if(NOT TARGET dpp)
    message(FATAL_ERROR "DPP target 'dpp' was not created.")
endif()

target_include_directories(dpp PRIVATE "${DISCORD_BRIDGE_DPP_DIR}/win32/include")
target_compile_definitions(dpp PRIVATE OPENSSL_SYS_WIN32 _WINSOCK_DEPRECATED_NO_WARNINGS _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE WIN32_LEAN_AND_MEAN NOMINMAX)

# =========================================================
# SHARED CORE
# =========================================================

add_library(DiscordBridgeCore STATIC
    src/core/BridgeCore.cpp
    src/discord/DiscordClient.cpp
    src/pawn/PawnRuntime.cpp
    src/pawn/natives/VersionNatives.cpp
    src/platform/openmp/OpenMPAdapter.cpp
    src/platform/samp/SAMPAdapter.cpp
)

target_include_directories(DiscordBridgeCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(DiscordBridgeCore PUBLIC dpp)
target_compile_definitions(DiscordBridgeCore PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# =========================================================
# OPEN.MP COMPONENT
# =========================================================

add_library(DiscordBridgeOpenMP SHARED
    src/platform/openmp/component/Component.cpp
    src/platform/openmp/component/EntryPoint.cpp
    src/pawn/natives/VersionNativesOpenMP.cpp
)

target_include_directories(DiscordBridgeOpenMP PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${DISCORD_BRIDGE_OPENMP_SDK_DIR}/include"
    "${DISCORD_BRIDGE_OPENMP_SDK_DIR}/lib/glm"
    "${DISCORD_BRIDGE_OPENMP_SDK_DIR}/lib/robin-hood-hashing/src/include"
    "${DISCORD_BRIDGE_OPENMP_SDK_DIR}/lib/span-lite/include"
    "${DISCORD_BRIDGE_OPENMP_SDK_DIR}/lib/string-view-lite/include"
    "${DISCORD_BRIDGE_OPENMP_PAWN_DIR}/source"
    "${DISCORD_BRIDGE_OPENMP_PAWN_DIR}/source/linux"
)

target_compile_definitions(DiscordBridgeOpenMP PRIVATE HAVE_STDINT_H=1 PAWN_CELL_SIZE=32 WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(DiscordBridgeOpenMP PRIVATE DiscordBridgeCore)

set_target_properties(DiscordBridgeOpenMP PROPERTIES
    OUTPUT_NAME "DiscordBridge"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/DiscordBridge/components/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/DiscordBridge/components/Release"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/DiscordBridge/components/Debug"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/DiscordBridge/components/Release"
    PDB_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/DiscordBridge/components/Debug"
    PDB_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/DiscordBridge/components/Release"
)

# =========================================================
# SA-MP PLUGIN
# =========================================================

add_library(DiscordBridgeSAMP SHARED
    src/platform/samp/plugin/Plugin.cpp
    src/platform/samp/plugin/AmxExports.cpp
    src/platform/samp/plugin/DiscordBridgeSAMP.def
    src/pawn/natives/VersionNativesSAMP.cpp
    "${DISCORD_BRIDGE_SAMP_SDK_DIR}/amxplugin.cpp"
)

target_include_directories(DiscordBridgeSAMP PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${DISCORD_BRIDGE_SAMP_SDK_DIR}"
    "${DISCORD_BRIDGE_SAMP_SDK_DIR}/amx"
)

target_compile_definitions(DiscordBridgeSAMP PRIVATE HAVE_STDINT_H WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(DiscordBridgeSAMP PRIVATE DiscordBridgeCore)

set_target_properties(DiscordBridgeSAMP PROPERTIES
    OUTPUT_NAME "DiscordBridge"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/DiscordBridge/plugins/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/DiscordBridge/plugins/Release"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/DiscordBridge/plugins/Debug"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/DiscordBridge/plugins/Release"
    PDB_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/DiscordBridge/plugins/Debug"
    PDB_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/DiscordBridge/plugins/Release"
)

# =========================================================
# RUNTIME DEPENDENCIES
# =========================================================

set(DISCORD_BRIDGE_DPP_RUNTIME_DIR "${DISCORD_BRIDGE_DPP_DIR}/win32/32/bin")
set(DISCORD_BRIDGE_RUNTIME_FILES
    "${DISCORD_BRIDGE_DPP_RUNTIME_DIR}/libssl-1_1.dll"
    "${DISCORD_BRIDGE_DPP_RUNTIME_DIR}/libcrypto-1_1.dll"
    "${DISCORD_BRIDGE_DPP_RUNTIME_DIR}/zlib1.dll"
)

foreach(RUNTIME_FILE IN LISTS DISCORD_BRIDGE_RUNTIME_FILES)
    if(NOT EXISTS "${RUNTIME_FILE}")
        message(FATAL_ERROR "DPP x86 runtime dependency not found: ${RUNTIME_FILE}")
    endif()
endforeach()

function(discordbridge_copy_runtime TARGET_NAME)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:dpp>" "$<TARGET_FILE_DIR:${TARGET_NAME}>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DISCORD_BRIDGE_RUNTIME_FILES} "$<TARGET_FILE_DIR:${TARGET_NAME}>"
        COMMAND_EXPAND_LISTS
        COMMENT "Copying DiscordBridge runtime dependencies for ${TARGET_NAME}"
        VERBATIM
    )
endfunction()

discordbridge_copy_runtime(DiscordBridgeOpenMP)
discordbridge_copy_runtime(DiscordBridgeSAMP)
