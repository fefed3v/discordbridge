#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace DiscordBridge
{
    class DiscordDataStore
    {
    public:
        void clear();

        void storeGuild(const std::string& guildId, const std::string& json);
        void storeChannel(const std::string& channelId, const std::string& json);
        void storeRole(const std::string& guildId, const std::string& roleId, const std::string& json);
        void storeMember(const std::string& guildId, const std::string& userId, const std::string& json);
        void storeUser(const std::string& userId, const std::string& json);

        bool getGuildName(const std::string& guildId, std::string& value) const;
        bool getGuildOwner(const std::string& guildId, std::string& value) const;
        bool getGuildMemberCount(const std::string& guildId, int& value) const;
        bool getGuildIcon(const std::string& guildId, std::string& value) const;
        bool getGuildIconUrl(const std::string& guildId, std::string& value) const;
        bool getGuildBanner(const std::string& guildId, std::string& value) const;
        bool getGuildBannerUrl(const std::string& guildId, std::string& value) const;
        bool getGuildDescription(const std::string& guildId, std::string& value) const;

        bool getChannelName(const std::string& channelId, std::string& value) const;
        bool getChannelType(const std::string& channelId, int& value) const;
        bool getChannelTopic(const std::string& channelId, std::string& value) const;
        bool getChannelParent(const std::string& channelId, std::string& value) const;
        bool getChannelGuild(const std::string& channelId, std::string& value) const;
        bool getChannelPosition(const std::string& channelId, int& value) const;
        bool getChannelNsfw(const std::string& channelId, bool& value) const;
        bool getChannelSlowmode(const std::string& channelId, int& value) const;

        bool getRoleName(const std::string& guildId, const std::string& roleId, std::string& value) const;
        bool getRoleColor(const std::string& guildId, const std::string& roleId, int& value) const;
        bool getRolePosition(const std::string& guildId, const std::string& roleId, int& value) const;
        bool getRoleHoist(const std::string& guildId, const std::string& roleId, bool& value) const;
        bool getRoleMentionable(const std::string& guildId, const std::string& roleId, bool& value) const;
        bool getRolePermissions(const std::string& guildId, const std::string& roleId, std::string& value) const;
        bool getRoleManaged(const std::string& guildId, const std::string& roleId, bool& value) const;

        bool getMemberNick(const std::string& guildId, const std::string& userId, std::string& value) const;
        bool getMemberName(const std::string& guildId, const std::string& userId, std::string& value) const;
        bool getMemberGlobalName(const std::string& guildId, const std::string& userId, std::string& value) const;
        bool getMemberAvatar(const std::string& guildId, const std::string& userId, std::string& value) const;
        bool memberHasRole(const std::string& guildId, const std::string& userId, const std::string& roleId) const;
        bool getMemberJoinedAt(const std::string& guildId, const std::string& userId, std::string& value) const;
        bool getMemberRoleCount(const std::string& guildId, const std::string& userId, int& value) const;
        bool getMemberRole(const std::string& guildId, const std::string& userId, int index, std::string& value) const;
        bool getMemberAvatarUrl(const std::string& guildId, const std::string& userId, std::string& value) const;

        bool getUserName(const std::string& userId, std::string& value) const;
        bool getUserGlobalName(const std::string& userId, std::string& value) const;
        bool getUserAvatar(const std::string& userId, std::string& value) const;
        bool isUserBot(const std::string& userId, bool& value) const;
        bool getUserAvatarUrl(const std::string& userId, std::string& value) const;

        static bool findObjectById(const std::string& jsonArray, const std::string& id, std::string& objectJson);
        static bool extractEmbeddedUser(const std::string& memberJson, std::string& userJson);

    private:
        static bool getString(const std::string& json, const std::string& key, std::string& value);
        static bool getInt(const std::string& json, const std::string& key, int& value);
        static bool getBool(const std::string& json, const std::string& key, bool& value);
        static bool getObject(const std::string& json, const std::string& key, std::string& value);
        static bool arrayHasString(const std::string& json, const std::string& key, const std::string& value);
        static bool getStringArray(const std::string& json, const std::string& key, std::vector<std::string>& values);
        static std::string cdnExtension(const std::string& hash);
        static std::string makePairKey(const std::string& first, const std::string& second);

        mutable std::mutex mutex_;
        std::unordered_map<std::string, std::string> guilds_;
        std::unordered_map<std::string, std::string> channels_;
        std::unordered_map<std::string, std::string> roles_;
        std::unordered_map<std::string, std::string> members_;
        std::unordered_map<std::string, std::string> users_;
    };
}
