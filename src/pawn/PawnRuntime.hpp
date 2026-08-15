#pragma once

#include <amx/amx.h>

#include <cstddef>
#include <string>
#include <vector>

namespace DiscordBridge
{
    class PawnRuntime
    {
    public:
        bool addAMX(AMX* amx);
        bool removeAMX(AMX* amx);

        void clear();

        void dispatchReady();
        void dispatchMessageCreate(const std::string& userId, const std::string& channelId, const std::string& message);
        void dispatchGuildMemberAdd(const std::string& guildId, const std::string& userId);
        void dispatchGuildMemberRemove(const std::string& guildId, const std::string& userId);

        void dispatchMessageSent(bool success, const std::string& channelId, const std::string& messageId);
        void dispatchMessageEdited(bool success, const std::string& channelId, const std::string& messageId);
        void dispatchMessageDeleted(bool success, const std::string& channelId, const std::string& messageId);
        void dispatchEmbedSent(bool success, const std::string& channelId, const std::string& messageId);
        void dispatchComponentsSent(bool success, const std::string& channelId, const std::string& messageId);

        void dispatchButtonClick(const std::string& userId, const std::string& channelId, const std::string& customId);

        std::size_t size() const;

    private:
        std::vector<AMX*> scripts_;
    };
}