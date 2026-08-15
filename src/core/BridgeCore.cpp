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
        discordClient_.shutdown();
        pawnRuntime_.clear();
        initialized_ = false;
    }

    void BridgeCore::process()
    {
        if (!initialized_) return;

        discordClient_.process();

        if (discordClient_.consumeReadyEvent())
        {
            pawnRuntime_.dispatchReady();
        }

        std::string userId;
        std::string channelId;
        std::string message;

        while (discordClient_.consumeMessageCreateEvent(userId, channelId, message))
        {
            pawnRuntime_.dispatchMessageCreate(userId, channelId, message);
        }

        std::string guildId;

        while (discordClient_.consumeGuildMemberAddEvent(guildId, userId))
        {
            pawnRuntime_.dispatchGuildMemberAdd(guildId, userId);
        }

        while (discordClient_.consumeGuildMemberRemoveEvent(guildId, userId))
        {
            pawnRuntime_.dispatchGuildMemberRemove(guildId, userId);
        }

        bool messageSuccess = false;
        std::string sentChannelId;
        std::string sentMessageId;

        while (discordClient_.consumeMessageSentEvent(
            messageSuccess,
            sentChannelId,
            sentMessageId
        ))
        {
            pawnRuntime_.dispatchMessageSent(
                messageSuccess,
                sentChannelId,
                sentMessageId
            );
        }

        bool operationSuccess = false;
        std::string operationChannelId;
        std::string operationMessageId;

        while (discordClient_.consumeMessageEditedEvent(operationSuccess, operationChannelId, operationMessageId))
        {
            pawnRuntime_.dispatchMessageEdited(operationSuccess, operationChannelId, operationMessageId);
        }

        while (discordClient_.consumeMessageDeletedEvent(operationSuccess, operationChannelId, operationMessageId))
        {
            pawnRuntime_.dispatchMessageDeleted(operationSuccess, operationChannelId, operationMessageId);
        }
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
}