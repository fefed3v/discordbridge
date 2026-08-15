#include "Plugin.hpp"
#include "AmxExports.hpp"

#include "../core/BridgeCore.hpp"
#include "../main/Version.hpp"
#include "../pawn/natives/BasicNatives.hpp"
#include "../pawn/natives/VersionNatives.hpp"
#include "../pawn/natives/PresenceNatives.hpp"

#include <plugincommon.h>
#include <amx/amx.h>

#include <memory>

namespace DiscordBridge
{
    using LogprintfFunction = void (*)(const char* format, ...);

    static LogprintfFunction logprintf = nullptr;
    static std::unique_ptr<BridgeCore> bridgeCore;

    BridgeCore* GetBridgeCore()
    {
        return bridgeCore.get();
    }
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
    return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void** ppData)
{
    if (ppData == nullptr) return false;

    DiscordBridge::logprintf = reinterpret_cast<DiscordBridge::LogprintfFunction>(ppData[PLUGIN_DATA_LOGPRINTF]);

    if (DiscordBridge::logprintf != nullptr) DiscordBridge::logprintf("[DiscordBridge] Loading Patch %s...", DiscordBridge::VERSION_STRING);

    if (!DiscordBridge::InitializeAmxExports(ppData))
    {
        if (DiscordBridge::logprintf != nullptr) DiscordBridge::logprintf("[DiscordBridge] Failed to initialize AMX exports.");
        DiscordBridge::logprintf = nullptr;
        return false;
    }

    DiscordBridge::bridgeCore = std::make_unique<DiscordBridge::BridgeCore>();

    if (!DiscordBridge::bridgeCore->initialize())
    {
        if (DiscordBridge::logprintf != nullptr) DiscordBridge::logprintf("[DiscordBridge] Failed to initialize BridgeCore.");

        DiscordBridge::bridgeCore.reset();
        DiscordBridge::ShutdownAmxExports();
        DiscordBridge::logprintf = nullptr;
        return false;
    }

    if (DiscordBridge::logprintf != nullptr) DiscordBridge::logprintf("[DiscordBridge] BridgeCore initialized.");

    return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
    if (DiscordBridge::bridgeCore)
    {
        DiscordBridge::bridgeCore->shutdown();
        DiscordBridge::bridgeCore.reset();
    }

    DiscordBridge::ShutdownAmxExports();

    if (DiscordBridge::logprintf != nullptr) DiscordBridge::logprintf("[DiscordBridge] Plugin unloaded.");

    DiscordBridge::logprintf = nullptr;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX* amx)
{
    if (amx == nullptr) return AMX_ERR_PARAMS;

    DiscordBridge::BridgeCore* core = DiscordBridge::GetBridgeCore();
    if (core == nullptr || !core->isInitialized()) return AMX_ERR_INIT;

    core->getPawnRuntime().addAMX(amx);

    const int versionResult = DiscordBridge::RegisterVersionNatives(amx);
    if (versionResult != AMX_ERR_NONE && versionResult != AMX_ERR_NOTFOUND)
    {
        core->getPawnRuntime().removeAMX(amx);
        return versionResult;
    }

    const int basicResult = DiscordBridge::RegisterBasicNatives(amx);
    if (basicResult != AMX_ERR_NONE && basicResult != AMX_ERR_NOTFOUND)
    {
        core->getPawnRuntime().removeAMX(amx);
        return basicResult;
    }

    const int presenceResult = DiscordBridge::RegisterPresenceNatives(amx);

    if (presenceResult != AMX_ERR_NONE && presenceResult != AMX_ERR_NOTFOUND)
    {
        core->getPawnRuntime().removeAMX(amx);
        return presenceResult;
    }

    return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX* amx)
{
    if (amx == nullptr) return AMX_ERR_PARAMS;

    DiscordBridge::BridgeCore* core = DiscordBridge::GetBridgeCore();

    if (core != nullptr) core->getPawnRuntime().removeAMX(amx);

    return AMX_ERR_NONE;
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick()
{
    DiscordBridge::BridgeCore* core = DiscordBridge::GetBridgeCore();

    if (core == nullptr || !core->isInitialized()) return;

    core->process();
}