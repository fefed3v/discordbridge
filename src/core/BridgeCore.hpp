#pragma once

#include "../discord/DiscordClient.hpp"
#include "../pawn/PawnRuntime.hpp"

namespace DiscordBridge
{
    class BridgeCore final
    {
    private:
        PawnRuntime pawnRuntime_;
        DiscordClient discordClient_;
        bool initialized_{false};

    public:
        BridgeCore() = default;
        ~BridgeCore();

        BridgeCore(const BridgeCore&) = delete;
        BridgeCore& operator=(const BridgeCore&) = delete;
        BridgeCore(BridgeCore&&) = delete;
        BridgeCore& operator=(BridgeCore&&) = delete;

        bool initialize();
        void shutdown();
        void process();

        bool isInitialized() const;

        PawnRuntime& getPawnRuntime();
        const PawnRuntime& getPawnRuntime() const;

        DiscordClient& getDiscordClient();
        const DiscordClient& getDiscordClient() const;
    };
}