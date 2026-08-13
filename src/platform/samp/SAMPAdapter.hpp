#pragma once

#include "../PlatformAdapter.hpp"

namespace DiscordBridge
{
    class SAMPAdapter final : public PlatformAdapter
    {
    public:
        SAMPAdapter() = default;
        ~SAMPAdapter() override = default;

        ServerPlatform getPlatform() const override;

        bool initialize() override;
        void shutdown() override;

        void process() override;

    private:
        bool initialized_ = false;
    };
}