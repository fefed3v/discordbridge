#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace DiscordBridge
{
    using CommandHandle = std::uint32_t;
    using CommandOptionHandle = std::uint32_t;

    struct CommandChoice
    {
        std::string name;
        std::string value;
        bool numeric{false};
    };

    struct CommandOption
    {
        CommandOptionHandle handle{0};
        int type{3};
        std::string name;
        std::string description;
        bool required{false};
        bool autocomplete{false};
        std::optional<double> minValue;
        std::optional<double> maxValue;
        int minLength{-1};
        int maxLength{-1};
        std::vector<int> channelTypes;
        std::vector<CommandChoice> choices;
        std::vector<CommandOption> options;

        bool isValid() const;
        std::string toJson() const;
    };

    class SlashCommand
    {
    public:
        SlashCommand(std::string name, std::string description);

        CommandOptionHandle addOption(CommandOption option, CommandOptionHandle parent = 0);
        CommandOption* findOption(CommandOptionHandle handle);
        bool addChoice(CommandOptionHandle option, CommandChoice choice);
        bool addChannelType(CommandOptionHandle option, int channelType);
        bool setAutocomplete(CommandOptionHandle option, bool enabled);
        bool setNumberRange(CommandOptionHandle option, double minValue, double maxValue);
        bool setStringLength(CommandOptionHandle option, int minLength, int maxLength);
        void setDefaultMemberPermissions(std::string permissions);
        void setNsfw(bool enabled);
        bool addContext(int context);
        bool addIntegrationType(int type);
        bool isValid() const;
        std::string toJson() const;

    private:
        CommandOption* findOptionRecursive(std::vector<CommandOption>& options, CommandOptionHandle handle);
        std::string name_;
        std::string description_;
        std::string defaultMemberPermissions_;
        bool nsfw_{false};
        std::vector<int> contexts_;
        std::vector<int> integrationTypes_;
        std::vector<CommandOption> options_;
        CommandOptionHandle nextOption_{1};
    };

    class CommandManager
    {
    public:
        CommandHandle create(const std::string& name, const std::string& description);
        bool destroy(CommandHandle handle);
        SlashCommand* get(CommandHandle handle);
        CommandOptionHandle addOption(CommandHandle handle, CommandOption option, CommandOptionHandle parent = 0);
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
