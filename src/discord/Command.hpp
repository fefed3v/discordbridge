#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace DiscordBridge
{
    using CommandHandle = std::uint32_t;

    struct CommandOption
    {
        int type{3};
        std::string name;
        std::string description;
        bool required{false};
    };

    class SlashCommand
    {
    public:
        SlashCommand(std::string name, std::string description);

        bool addOption(const CommandOption& option);
        bool isValid() const;
        std::string toJson() const;

    private:
        std::string name_;
        std::string description_;
        std::vector<CommandOption> options_;
    };

    class CommandManager
    {
    public:
        CommandHandle create(const std::string& name, const std::string& description);
        bool destroy(CommandHandle handle);
        SlashCommand* get(CommandHandle handle);
        bool addOption(CommandHandle handle, const CommandOption& option);
        void clear();

        void setGuild(std::string guildId, bool autoDeploy);
        const std::string& guildId() const;
        bool autoDeploy() const;
        bool dirty() const;
        void markDirty();
        void markDeployed();
        std::string toJson() const;

    private:
        std::unordered_map<CommandHandle, std::unique_ptr<SlashCommand>> commands_;
        CommandHandle next_{1};
        std::string guildId_;
        bool autoDeploy_{true};
        bool dirty_{false};
    };
}
