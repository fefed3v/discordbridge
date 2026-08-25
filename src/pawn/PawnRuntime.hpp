#pragma once

#include <amx/amx.h>

#include <cstddef>
#include <string>
#include <vector>
#include <utility>

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

        void dispatchButtonClick(const std::string& userId, const std::string& channelId, const std::string& customId, const std::string& interactionId, const std::string& interactionToken);
        void dispatchSelectMenu(const std::string& userId, const std::string& channelId, const std::string& customId, const std::string& value, const std::string& interactionId, const std::string& interactionToken);
        void dispatchModalSubmit(const std::string& userId, const std::string& channelId, const std::string& customId, const std::vector<std::pair<std::string, std::string>>& values, const std::string& interactionId, const std::string& interactionToken);
        bool getModalValue(const std::string& customId, std::string& value) const;

        std::size_t size() const;

    private:
        std::vector<AMX*> scripts_;
        const std::vector<std::pair<std::string, std::string>>* activeModalValues_{nullptr};
    };
}