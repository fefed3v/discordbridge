#pragma once

#include "../discord/DiscordClient.hpp"
#include "../discord/embed/EmbedManager.hpp"
#include "../pawn/PawnRuntime.hpp"

namespace DiscordBridge
{
    class BridgeCore final
    {
    private:
        bool initialized_{false};

        PawnRuntime pawnRuntime_;
        DiscordClient discordClient_;
        EmbedManager embedManager_;

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

        EmbedManager& getEmbedManager();
        const EmbedManager& getEmbedManager() const;
    };
}