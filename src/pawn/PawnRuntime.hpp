#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <amx/amx.h>

namespace DiscordBridge
{
    class PawnRuntime final
    {
    private:
        std::vector<AMX*> scripts_;

    public:
        PawnRuntime() = default;
        ~PawnRuntime() = default;

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

        std::size_t size() const;
    };
}