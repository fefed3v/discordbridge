#pragma once

#include "../discord/components/legacy/Button.hpp"
#include "../discord/client/DiscordClient.hpp"
#include "../discord/embeds/Embed.hpp"
#include "../discord/components/legacy/SelectMenu.hpp"
#include "../discord/components/legacy/Modal.hpp"
#include "../discord/commands/Command.hpp"
#include "../discord/components/v2/ComponentsV2.hpp"
#include "../discord/components/v2/Component.hpp"
#include "../pawn/PawnRuntime.hpp"
#include <chrono>

namespace DiscordBridge
{
    class BridgeCore
    {
    public:
        BridgeCore() = default;
        ~BridgeCore();

        bool initialize();
        void shutdown();
        void process();

        bool isInitialized() const;

        PawnRuntime &getPawnRuntime();
        const PawnRuntime &getPawnRuntime() const;

        DiscordClient &getDiscordClient();
        const DiscordClient &getDiscordClient() const;

        EmbedManager &getEmbedManager();
        const EmbedManager &getEmbedManager() const;

        ButtonManager &getButtonManager();
        const ButtonManager &getButtonManager() const;

        ActionRowManager &getActionRowManager();
        const ActionRowManager &getActionRowManager() const;
        SelectMenuManager &getSelectMenuManager();
        ModalManager &getModalManager();
        CommandManager &getCommandManager();
        ComponentsV2Manager &getV2Manager();
        ComponentManager &getComponentManager();

    private:
        bool initialized_{false};
        std::chrono::steady_clock::time_point lastDeployAttempt_{};

        PawnRuntime pawnRuntime_;
        DiscordClient discordClient_;
        EmbedManager embedManager_;
        ButtonManager buttonManager_;
        ActionRowManager actionRowManager_;
        SelectMenuManager selectMenuManager_;
        ModalManager modalManager_;
        CommandManager commandManager_;
        ComponentsV2Manager v2Manager_;
        ComponentManager componentManager_;
    };
}