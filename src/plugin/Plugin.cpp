#include "Plugin.hpp"

#include "AmxExports.hpp"
#include "../Version.hpp"
#include "../core/BridgeCore.hpp"
#include "../pawn/Natives.hpp"

#include <plugincommon.h>

#include <memory>

using namespace DiscordBridge;
using namespace std;

namespace DiscordBridge
{
    using LogprintfFunction = void (*)(const char* format, ...);

    namespace
    {
        LogprintfFunction logprintf = nullptr;
        unique_ptr<BridgeCore> bridgeCore;

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

    logprintf = reinterpret_cast<LogprintfFunction>(ppData[PLUGIN_DATA_LOGPRINTF]);

    if (logprintf) logprintf("[DiscordBridge] Loading Patch %s...", VERSION_STRING);

    if (!InitializeAmxExports(ppData))
    {
        Log("[DiscordBridge] Failed to initialize AMX exports.");
        logprintf = nullptr;
        return false;
    }

    bridgeCore = make_unique<BridgeCore>();

    if (!bridgeCore->initialize())
    {
        Log("[DiscordBridge] Failed to initialize BridgeCore.");
        bridgeCore.reset();
        ShutdownAmxExports();
        logprintf = nullptr;
        return false;
    }

    Log("[DiscordBridge] BridgeCore initialized.");
    return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
    if (bridgeCore)
    {
        bridgeCore->shutdown();
        bridgeCore.reset();
    }

    ShutdownAmxExports();
    Log("[DiscordBridge] Plugin unloaded.");
    logprintf = nullptr;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX* amx)
{
    if (!amx) return AMX_ERR_PARAMS;

    BridgeCore* core = GetBridgeCore();
    if (!core || !core->isInitialized()) return AMX_ERR_INIT;

    if (!core->getPawnRuntime().addAMX(amx)) return AMX_ERR_NONE;

    const int result = RegisterNatives(amx);

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

    if (BridgeCore* core = GetBridgeCore()) core->getPawnRuntime().removeAMX(amx);

    return AMX_ERR_NONE;
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick()
{
    BridgeCore* core = GetBridgeCore();
    if (core && core->isInitialized()) core->process();
}