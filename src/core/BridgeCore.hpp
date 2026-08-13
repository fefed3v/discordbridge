#pragma once

#include "../discord/DiscordClient.hpp"
#include "../pawn/PawnRuntime.hpp"
#include "../platform/PlatformAdapter.hpp"

#include <memory>

namespace DiscordBridge
{
    class BridgeCore final
    {
    public:
        BridgeCore() = default;
        ~BridgeCore();

        BridgeCore(const BridgeCore&) = delete;
        BridgeCore& operator=(const BridgeCore&) = delete;
        BridgeCore(BridgeCore&&) = delete;
        BridgeCore& operator=(BridgeCore&&) = delete;

        bool initialize(std::unique_ptr<PlatformAdapter> platform);
        void shutdown();
        void process();

        bool isInitialized() const;
        ServerPlatform getPlatform() const;

        PawnRuntime& getPawnRuntime();
        const PawnRuntime& getPawnRuntime() const;
        DiscordClient& getDiscordClient();
        const DiscordClient& getDiscordClient() const;

    private:
        std::unique_ptr<PlatformAdapter> platform_;
        PawnRuntime pawnRuntime_;
        DiscordClient discordClient_;
        bool initialized_{false};
    };
}
