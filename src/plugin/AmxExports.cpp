#include "AmxExports.hpp"

#include <plugincommon.h>

extern void *pAMXFunctions;
using namespace DiscordBridge;

bool DiscordBridge::InitializeAmxExports(void **pluginData)
{
    if (pluginData == nullptr)
        return false;

    void *functions = pluginData[PLUGIN_DATA_AMX_EXPORTS];
    if (functions == nullptr)
        return false;

    pAMXFunctions = functions;
    return true;
}

void DiscordBridge::ShutdownAmxExports()
{
    pAMXFunctions = nullptr;
}

bool DiscordBridge::AreAmxExportsInitialized()
{
    return pAMXFunctions != nullptr;
}