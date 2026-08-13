#pragma once

namespace DiscordBridge
{
    bool InitializeAmxExports(void** pluginData);

    void ShutdownAmxExports();

    bool AreAmxExportsInitialized();
}