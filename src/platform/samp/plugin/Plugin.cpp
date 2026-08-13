#include "Plugin.hpp"
#include "AmxExports.hpp"

#include "../../../main/Version.hpp"
#include "../../../pawn/natives/VersionNatives.hpp"

#include <plugincommon.h>
#include <amx/amx.h>

#include <memory>
#include <utility>

namespace DiscordBridge
{
    using LogprintfFunction = void (*)(const char* format, ...);

    static LogprintfFunction logprintf = nullptr;
    static std::unique_ptr<BridgeCore> bridgeCore;

    static bool IsCoreReady()
    {
        return bridgeCore != nullptr && bridgeCore->isInitialized();
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
    if (DiscordBridge::logprintf == nullptr) return false;

    DiscordBridge::logprintf("[DiscordBridge] Loading Patch %s...", DiscordBridge::VERSION_STRING);

    if (!DiscordBridge::InitializeAmxExports(ppData))
    {
        DiscordBridge::logprintf("[DiscordBridge] Failed to initialize AMX exports.");
        DiscordBridge::logprintf = nullptr;
        return false;
    }

    DiscordBridge::bridgeCore = std::make_unique<DiscordBridge::BridgeCore>();
    auto adapter = std::make_unique<DiscordBridge::SAMPAdapter>();

    if (!DiscordBridge::bridgeCore->initialize(std::move(adapter)))
    {
        DiscordBridge::logprintf("[DiscordBridge] Failed to initialize BridgeCore.");
        DiscordBridge::bridgeCore.reset();
        DiscordBridge::ShutdownAmxExports();
        DiscordBridge::logprintf = nullptr;
        return false;
    }

    DiscordBridge::logprintf("[DiscordBridge] BridgeCore initialized.");
    DiscordBridge::logprintf("[DiscordBridge] Platform: SA-MP.");
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

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick()
{
    if (DiscordBridge::IsCoreReady()) DiscordBridge::bridgeCore->process();
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX* amx)
{
    if (amx == nullptr) return AMX_ERR_PARAMS;
    if (!DiscordBridge::IsCoreReady()) return AMX_ERR_INIT;

    if (!DiscordBridge::bridgeCore->getPawnRuntime().addAMX(amx)) return AMX_ERR_NONE;

    const int result = DiscordBridge::RegisterVersionNativesSAMP(amx);
    if (result != AMX_ERR_NONE)
    {
        DiscordBridge::bridgeCore->getPawnRuntime().removeAMX(amx);
        return result;
    }

    return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX* amx)
{
    if (amx == nullptr) return AMX_ERR_PARAMS;
    if (!DiscordBridge::bridgeCore) return AMX_ERR_NONE;

    DiscordBridge::bridgeCore->getPawnRuntime().removeAMX(amx);
    return AMX_ERR_NONE;
}
