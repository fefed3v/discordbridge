#include "BridgeCore.hpp"

#include <utility>
#include <vector>

using namespace DiscordBridge;
using namespace std;

BridgeCore::~BridgeCore()
{
    shutdown();
}

bool BridgeCore::initialize()
{
    if (initialized_)
        return false;

    initialized_ = true;
    return true;
}

void BridgeCore::shutdown()
{
    if (!initialized_)
        return;

    discordClient_.shutdown();

    commandManager_.clear();
    v2Manager_.clear();
    modalManager_.clear();
    selectMenuManager_.clear();
    actionRowManager_.clear();
    buttonManager_.clear();
    embedManager_.clear();
    pawnRuntime_.clear();

    initialized_ = false;
}

void BridgeCore::process()
{
    if (!initialized_)
        return;

    discordClient_.process();

    if (commandManager_.autoDeploy() && commandManager_.dirty() && discordClient_.isConnected() && !commandManager_.guildId().empty())
    {
        if (discordClient_.deployCommands(commandManager_.guildId(), commandManager_.toJson())) commandManager_.markDeployed();
    }

    if (discordClient_.consumeReadyEvent())
        pawnRuntime_.dispatchReady();

    string userId;
    string channelId;
    string message;

    while (discordClient_.consumeMessageCreateEvent(userId, channelId, message))
        pawnRuntime_.dispatchMessageCreate(userId, channelId, message);

    string guildId;

    while (discordClient_.consumeGuildMemberAddEvent(guildId, userId))
        pawnRuntime_.dispatchGuildMemberAdd(guildId, userId);
    while (discordClient_.consumeGuildMemberRemoveEvent(guildId, userId))
        pawnRuntime_.dispatchGuildMemberRemove(guildId, userId);

    string interactionId;
    string interactionToken;
    string customId;

    while (discordClient_.consumeButtonClickEvent(interactionId, interactionToken, userId, channelId, customId))
    {
        pawnRuntime_.dispatchButtonClick(userId, channelId, customId, interactionId, interactionToken);
        discordClient_.acknowledgeInteractionAsync(interactionId, interactionToken);
    }

    string value;
    vector<pair<string, string>> commandOptions;
    string commandName;

    while (discordClient_.consumeSlashCommandEvent(interactionId, interactionToken, userId, guildId, channelId, commandName, commandOptions))
    {
        pawnRuntime_.dispatchSlashCommand(commandName, userId, guildId, channelId, commandOptions, interactionId, interactionToken);
        discordClient_.deferInteractionAsync(interactionId, interactionToken, true);
    }

    while (discordClient_.consumeSelectMenuEvent(interactionId, interactionToken, userId, channelId, customId, value))
    {
        pawnRuntime_.dispatchSelectMenu(userId, channelId, customId, value, interactionId, interactionToken);
        discordClient_.acknowledgeInteractionAsync(interactionId, interactionToken);
    }

    vector<pair<string, string>> modalValues;
    while (discordClient_.consumeModalEvent(interactionId, interactionToken, userId, channelId, customId, modalValues))
    {
        pawnRuntime_.dispatchModalSubmit(userId, channelId, customId, modalValues, interactionId, interactionToken);
        discordClient_.deferInteractionAsync(interactionId, interactionToken, true);
    }

    bool success = false;
    string messageId;

    while (discordClient_.consumeMessageSentEvent(success, channelId, messageId))
        pawnRuntime_.dispatchMessageSent(success, channelId, messageId);
    while (discordClient_.consumeMessageEditedEvent(success, channelId, messageId))
        pawnRuntime_.dispatchMessageEdited(success, channelId, messageId);
    while (discordClient_.consumeMessageDeletedEvent(success, channelId, messageId))
        pawnRuntime_.dispatchMessageDeleted(success, channelId, messageId);
    while (discordClient_.consumeEmbedSentEvent(success, channelId, messageId))
        pawnRuntime_.dispatchEmbedSent(success, channelId, messageId);
    while (discordClient_.consumeComponentsSentEvent(success, channelId, messageId))
        pawnRuntime_.dispatchComponentsSent(success, channelId, messageId);
    while (discordClient_.consumeV2SentEvent(success, channelId, messageId))
        pawnRuntime_.dispatchV2Sent(success, channelId, messageId);

    while (discordClient_.consumeV2EditedEvent(success, channelId, messageId))
        pawnRuntime_.dispatchV2Edited(success, channelId, messageId);

    string targetId;
    while (discordClient_.consumeChannelCreatedEvent(success, guildId, targetId)) pawnRuntime_.dispatchChannelCreated(success, guildId, targetId);
    while (discordClient_.consumeChannelDeletedEvent(success, guildId, targetId)) pawnRuntime_.dispatchChannelDeleted(success, guildId, targetId);
    while (discordClient_.consumeRoleCreatedEvent(success, guildId, targetId)) pawnRuntime_.dispatchRoleCreated(success, guildId, targetId);
    while (discordClient_.consumeRoleDeletedEvent(success, guildId, targetId)) pawnRuntime_.dispatchRoleDeleted(success, guildId, targetId);
    while (discordClient_.consumeMemberRoleAddedEvent(success, guildId, targetId)) pawnRuntime_.dispatchMemberRoleAdded(success, guildId, targetId);
    while (discordClient_.consumeMemberRoleRemovedEvent(success, guildId, targetId)) pawnRuntime_.dispatchMemberRoleRemoved(success, guildId, targetId);
    while (discordClient_.consumeMemberKickedEvent(success, guildId, targetId)) pawnRuntime_.dispatchMemberKicked(success, guildId, targetId);
    while (discordClient_.consumeMemberBannedEvent(success, guildId, targetId)) pawnRuntime_.dispatchMemberBanned(success, guildId, targetId);
    while (discordClient_.consumeMemberUnbannedEvent(success, guildId, targetId)) pawnRuntime_.dispatchMemberUnbanned(success, guildId, targetId);
    while (discordClient_.consumeCommandsDeployedEvent(success, guildId)) pawnRuntime_.dispatchCommandsDeployed(success, guildId);

    while (discordClient_.consumeGuildFetchedEvent(success, guildId)) pawnRuntime_.dispatchGuildFetched(success, guildId);
    while (discordClient_.consumeChannelFetchedEvent(success, channelId)) pawnRuntime_.dispatchChannelFetched(success, channelId);
    while (discordClient_.consumeRoleFetchedEvent(success, guildId, targetId)) pawnRuntime_.dispatchRoleFetched(success, guildId, targetId);
    while (discordClient_.consumeMemberFetchedEvent(success, guildId, targetId)) pawnRuntime_.dispatchMemberFetched(success, guildId, targetId);
    while (discordClient_.consumeUserFetchedEvent(success, targetId)) pawnRuntime_.dispatchUserFetched(success, targetId);
}

bool BridgeCore::isInitialized() const
{
    return initialized_;
}

PawnRuntime &BridgeCore::getPawnRuntime()
{
    return pawnRuntime_;
}

const PawnRuntime &BridgeCore::getPawnRuntime() const
{
    return pawnRuntime_;
}

DiscordClient &BridgeCore::getDiscordClient()
{
    return discordClient_;
}

const DiscordClient &BridgeCore::getDiscordClient() const
{
    return discordClient_;
}

EmbedManager &BridgeCore::getEmbedManager()
{
    return embedManager_;
}

const EmbedManager &BridgeCore::getEmbedManager() const
{
    return embedManager_;
}

ButtonManager &BridgeCore::getButtonManager()
{
    return buttonManager_;
}

const ButtonManager &BridgeCore::getButtonManager() const
{
    return buttonManager_;
}

ActionRowManager &BridgeCore::getActionRowManager()
{
    return actionRowManager_;
}

const ActionRowManager &BridgeCore::getActionRowManager() const
{
    return actionRowManager_;
}

SelectMenuManager &BridgeCore::getSelectMenuManager() { return selectMenuManager_; }
ModalManager &BridgeCore::getModalManager() { return modalManager_; }
CommandManager &BridgeCore::getCommandManager() { return commandManager_; }

ComponentsV2Manager &BridgeCore::getV2Manager() { return v2Manager_; }
