#pragma once

#include "../main/Platform.hpp"

namespace DiscordBridge
{
    class PlatformAdapter
    {
    public:
        virtual ~PlatformAdapter() = default;

        virtual ServerPlatform getPlatform() const = 0;

        virtual bool initialize() = 0;
        virtual void shutdown() = 0;

        virtual void process() = 0;
    };
}