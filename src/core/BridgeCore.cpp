#include "BridgeCore.hpp"

namespace DiscordBridge
{
    BridgeCore::~BridgeCore()
    {
        shutdown();
    }

    bool BridgeCore::initialize()
    {
        if (initialized_) return false;

        initialized_ = true;
        return true;
    }

    void BridgeCore::shutdown()
    {
        if (!initialized_) return;

        discordClient_.shutdown();

        actionRowManager_.clear();
        buttonManager_.clear();
        embedManager_.clear();
        pawnRuntime_.clear();

        initialized_ = false;
    }

    void BridgeCore::process()
    {
        if (!initialized_) return;

        discordClient_.process();

        if (discordClient_.consumeReadyEvent()) pawnRuntime_.dispatchReady();

        std::string userId;
        std::string channelId;
        std::string message;

        while (discordClient_.consumeMessageCreateEvent(userId, channelId, message)) pawnRuntime_.dispatchMessageCreate(userId, channelId, message);

        std::string guildId;

        while (discordClient_.consumeGuildMemberAddEvent(guildId, userId)) pawnRuntime_.dispatchGuildMemberAdd(guildId, userId);
        while (discordClient_.consumeGuildMemberRemoveEvent(guildId, userId)) pawnRuntime_.dispatchGuildMemberRemove(guildId, userId);

        std::string interactionId;
        std::string interactionToken;
        std::string customId;

        while (discordClient_.consumeButtonClickEvent(interactionId, interactionToken, userId, channelId, customId)) pawnRuntime_.dispatchButtonClick(userId, channelId, customId);

        bool success = false;
        std::string messageId;

        while (discordClient_.consumeMessageSentEvent(success, channelId, messageId)) pawnRuntime_.dispatchMessageSent(success, channelId, messageId);
        while (discordClient_.consumeMessageEditedEvent(success, channelId, messageId)) pawnRuntime_.dispatchMessageEdited(success, channelId, messageId);
        while (discordClient_.consumeMessageDeletedEvent(success, channelId, messageId)) pawnRuntime_.dispatchMessageDeleted(success, channelId, messageId);
        while (discordClient_.consumeEmbedSentEvent(success, channelId, messageId)) pawnRuntime_.dispatchEmbedSent(success, channelId, messageId);
        while (discordClient_.consumeComponentsSentEvent(success, channelId, messageId)) pawnRuntime_.dispatchComponentsSent(success, channelId, messageId);
    }

    bool BridgeCore::isInitialized() const
    {
        return initialized_;
    }

    PawnRuntime& BridgeCore::getPawnRuntime()
    {
        return pawnRuntime_;
    }

    const PawnRuntime& BridgeCore::getPawnRuntime() const
    {
        return pawnRuntime_;
    }

    DiscordClient& BridgeCore::getDiscordClient()
    {
        return discordClient_;
    }

    const DiscordClient& BridgeCore::getDiscordClient() const
    {
        return discordClient_;
    }

    EmbedManager& BridgeCore::getEmbedManager()
    {
        return embedManager_;
    }

    const EmbedManager& BridgeCore::getEmbedManager() const
    {
        return embedManager_;
    }

    ButtonManager& BridgeCore::getButtonManager()
    {
        return buttonManager_;
    }

    const ButtonManager& BridgeCore::getButtonManager() const
    {
        return buttonManager_;
    }

    ActionRowManager& BridgeCore::getActionRowManager()
    {
        return actionRowManager_;
    }

    const ActionRowManager& BridgeCore::getActionRowManager() const
    {
        return actionRowManager_;
    }
}