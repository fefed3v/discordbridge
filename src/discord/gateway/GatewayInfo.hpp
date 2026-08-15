#pragma once

#include <string>

namespace DiscordBridge
{
    struct GatewayInfo
    {
        std::string url;
        int shards{1};
        int sessionTotal{0};
        int sessionRemaining{0};
        int sessionResetAfter{0};
        int maxConcurrency{1};

        bool isValid() const;
    };

    bool ParseGatewayInfo(const std::string& json, GatewayInfo& info);
}