#include "Plugin.hpp"

#include "AmxExports.hpp"
#include "../Version.hpp"
#include "../core/BridgeCore.hpp"
#include "../pawn/Natives.hpp"

#include <plugincommon.h>

#include <memory>

namespace DiscordBridge
{
    using LogprintfFunction = void (*)(const char* format, ...);

    namespace
    {
        LogprintfFunction logprintf = nullptr;
        std::unique_ptr<BridgeCore> bridgeCore;

        void Log(const char* message)
        {
            if (logprintf) logprintf("%s", message);
        }
    }

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
    if (!ppData) return false;

    DiscordBridge::logprintf = reinterpret_cast<DiscordBridge::LogprintfFunction>(ppData[PLUGIN_DATA_LOGPRINTF]);

    if (DiscordBridge::logprintf) DiscordBridge::logprintf("[DiscordBridge] Loading Patch %s...", DiscordBridge::VERSION_STRING);

    if (!DiscordBridge::InitializeAmxExports(ppData))
    {
        DiscordBridge::Log("[DiscordBridge] Failed to initialize AMX exports.");
        DiscordBridge::logprintf = nullptr;
        return false;
    }

    DiscordBridge::bridgeCore = std::make_unique<DiscordBridge::BridgeCore>();

    if (!DiscordBridge::bridgeCore->initialize())
    {
        DiscordBridge::Log("[DiscordBridge] Failed to initialize BridgeCore.");
        DiscordBridge::bridgeCore.reset();
        DiscordBridge::ShutdownAmxExports();
        DiscordBridge::logprintf = nullptr;
        return false;
    }

    DiscordBridge::Log("[DiscordBridge] BridgeCore initialized.");
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
    DiscordBridge::Log("[DiscordBridge] Plugin unloaded.");
    DiscordBridge::logprintf = nullptr;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX* amx)
{
    if (!amx) return AMX_ERR_PARAMS;

    DiscordBridge::BridgeCore* core = DiscordBridge::GetBridgeCore();
    if (!core || !core->isInitialized()) return AMX_ERR_INIT;

    if (!core->getPawnRuntime().addAMX(amx)) return AMX_ERR_NONE;

    const int result = DiscordBridge::RegisterNatives(amx);

    if (result != AMX_ERR_NONE && result != AMX_ERR_NOTFOUND)
    {
        core->getPawnRuntime().removeAMX(amx);
        return result;
    }

    return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX* amx)
{
    if (!amx) return AMX_ERR_PARAMS;

    if (DiscordBridge::BridgeCore* core = DiscordBridge::GetBridgeCore()) core->getPawnRuntime().removeAMX(amx);

    return AMX_ERR_NONE;
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick()
{
    DiscordBridge::BridgeCore* core = DiscordBridge::GetBridgeCore();
    if (core && core->isInitialized()) core->process();
}