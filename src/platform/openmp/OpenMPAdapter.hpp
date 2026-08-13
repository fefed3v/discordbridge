#pragma once

#include "../PlatformAdapter.hpp"

namespace DiscordBridge
{
    class OpenMPAdapter final : public PlatformAdapter
    {
    public:
        OpenMPAdapter() = default;
        ~OpenMPAdapter() override = default;

        ServerPlatform getPlatform() const override;

        bool initialize() override;
        void shutdown() override;

        void process() override;

    private:
        bool initialized_ = false;
    };
}