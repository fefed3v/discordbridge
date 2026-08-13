#include "AmxExports.hpp"

#include <plugincommon.h>

extern void* pAMXFunctions;

namespace DiscordBridge
{
    bool InitializeAmxExports(void** pluginData)
    {
        if (pluginData == nullptr) return false;

        void* functions = pluginData[PLUGIN_DATA_AMX_EXPORTS];
        if (functions == nullptr) return false;

        pAMXFunctions = functions;
        return true;
    }

    void ShutdownAmxExports()
    {
        pAMXFunctions = nullptr;
    }

    bool AreAmxExportsInitialized()
    {
        return pAMXFunctions != nullptr;
    }
}
