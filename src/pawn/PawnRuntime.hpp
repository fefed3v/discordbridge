#pragma once

#include <amx/amx.h>

#include <cstddef>
#include <string>
#include <vector>
#include <utility>

using namespace std;

namespace DiscordBridge
{
    class PawnRuntime
    {
    public:
        bool addAMX(AMX *amx);
        bool removeAMX(AMX *amx);

        void clear();

        void dispatchReady();
        void dispatchMessageCreate(const string &userId, const string &channelId, const string &message);
        void dispatchGuildMemberAdd(const string &guildId, const string &userId);
        void dispatchGuildMemberRemove(const string &guildId, const string &userId);

        void dispatchMessageSent(bool success, const string &channelId, const string &messageId);
        void dispatchMessageEdited(bool success, const string &channelId, const string &messageId);
        void dispatchMessageDeleted(bool success, const string &channelId, const string &messageId);
        void dispatchEmbedSent(bool success, const string &channelId, const string &messageId);
        void dispatchComponentsSent(bool success, const string &channelId, const string &messageId);
        void dispatchV2Sent(bool success, const string &channelId, const string &messageId);
        void dispatchV2Edited(bool success, const string &channelId, const string &messageId);
        void dispatchChannelCreated(bool success, const string &guildId, const string &channelId);
        void dispatchChannelDeleted(bool success, const string &guildId, const string &channelId);
        void dispatchRoleCreated(bool success, const string &guildId, const string &roleId);
        void dispatchRoleDeleted(bool success, const string &guildId, const string &roleId);
        void dispatchMemberRoleAdded(bool success, const string &guildId, const string &userId);
        void dispatchMemberRoleRemoved(bool success, const string &guildId, const string &userId);
        void dispatchMemberKicked(bool success, const string &guildId, const string &userId);
        void dispatchMemberBanned(bool success, const string &guildId, const string &userId);
        void dispatchMemberUnbanned(bool success, const string &guildId, const string &userId);
        void dispatchCommandsDeployed(bool success, const string &guildId);
        void dispatchGuildFetched(bool success, const string &guildId);
        void dispatchChannelFetched(bool success, const string &channelId);
        void dispatchRoleFetched(bool success, const string &guildId, const string &roleId);
        void dispatchMemberFetched(bool success, const string &guildId, const string &userId);
        void dispatchUserFetched(bool success, const string &userId);

        void dispatchButtonClick(const string &userId, const string &guildId, const string &channelId, const string &customId, const string &interactionId, const string &interactionToken);
        void dispatchSelectMenu(const string &userId, const string &guildId, const string &channelId, const string &customId, const vector<string> &values, const string &interactionId, const string &interactionToken);
        void dispatchModalSubmit(const string &userId, const string &guildId, const string &channelId, const string &customId, const vector<pair<string, string>> &values, const string &interactionId, const string &interactionToken);
        bool getModalValue(const string &customId, string &value) const;
        void dispatchSlashCommand(const string &commandName, const string &userId, const string &guildId, const string &channelId, const vector<pair<string, string>> &options, const string &interactionId, const string &interactionToken);
        void dispatchAutocomplete(const string &commandName, const string &userId, const string &guildId, const string &channelId, const vector<pair<string, string>> &options, const string &interactionId, const string &interactionToken);
        bool getCommandValue(const string &name, string &value) const;
        size_t getInteractionValueCount() const;
        bool getInteractionValue(size_t index, string &value) const;
        bool getInteractionField(const string &name, string &value) const;

        size_t size() const;

    private:
        vector<AMX *> scripts_;
        const vector<pair<string, string>> *activeModalValues_{nullptr};
        const vector<pair<string, string>> *activeCommandValues_{nullptr};
        const vector<string> *activeInteractionValues_{nullptr};
    };
}