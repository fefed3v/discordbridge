#include "Command.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>
#include <utility>

namespace DiscordBridge
{
    namespace
    {
        std::string EscapeJson(const std::string &value)
        {
            std::string out;
            out.reserve(value.size());
            for (const char c : value)
            {
                switch (c)
                {
                case '"':
                    out += "\\\"";
                    break;
                case '\\':
                    out += "\\\\";
                    break;
                case '\n':
                    out += "\\n";
                    break;
                case '\r':
                    out += "\\r";
                    break;
                case '\t':
                    out += "\\t";
                    break;
                default:
                    out += c;
                    break;
                }
            }
            return out;
        }

        bool ValidName(const std::string &value)
        {
            if (value.empty() || value.size() > 32)
                return false;
            for (const unsigned char c : value)
                if (!(std::islower(c) || std::isdigit(c) || c == '-' || c == '_' || c == '\''))
                    return false;
            return true;
        }

        template <class T>
        bool Contains(const std::vector<T> &values, const T value) { return std::find(values.begin(), values.end(), value) != values.end(); }
        std::string Number(double value)
        {
            std::ostringstream out;
            out.precision(15);
            out << value;
            return out.str();
        }
        std::string IntArray(const std::vector<int> &values)
        {
            std::string json = "[";
            for (std::size_t i = 0; i < values.size(); ++i)
            {
                if (i)
                    json += ',';
                json += std::to_string(values[i]);
            }
            return json + ']';
        }
    }

    bool CommandOption::isValid() const
    {
        if (!ValidName(name) || description.empty() || description.size() > 100 || type < 1 || type > 11)
            return false;
        if ((type == 1 || type == 2) && required)
            return false;
        if (autocomplete && type != 3 && type != 4 && type != 10)
            return false;
        if (autocomplete && !choices.empty())
            return false;
        if (!choices.empty() && type != 3 && type != 4 && type != 10)
            return false;
        if (!channelTypes.empty() && type != 7)
            return false;
        if ((minValue || maxValue) && type != 4 && type != 10)
            return false;
        if ((minLength >= 0 || maxLength >= 0) && type != 3)
            return false;
        if (!options.empty() && type != 1 && type != 2)
            return false;
        return choices.size() <= 25 && options.size() <= 25;
    }

    std::string CommandOption::toJson() const
    {
        if (!isValid())
            return {};
        std::string json = "{\"type\":" + std::to_string(type) + ",\"name\":\"" + EscapeJson(name) + "\",\"description\":\"" + EscapeJson(description) + "\"";
        if (type != 1 && type != 2)
            json += ",\"required\":" + std::string(required ? "true" : "false");
        if (autocomplete)
            json += ",\"autocomplete\":true";
        if (minValue)
            json += ",\"min_value\":" + Number(*minValue);
        if (maxValue)
            json += ",\"max_value\":" + Number(*maxValue);
        if (minLength >= 0)
            json += ",\"min_length\":" + std::to_string(minLength);
        if (maxLength >= 0)
            json += ",\"max_length\":" + std::to_string(maxLength);
        if (!channelTypes.empty())
            json += ",\"channel_types\":" + IntArray(channelTypes);
        if (!choices.empty())
        {
            json += ",\"choices\":[";
            for (std::size_t i = 0; i < choices.size(); ++i)
            {
                if (i)
                    json += ',';
                json += "{\"name\":\"" + EscapeJson(choices[i].name) + "\",\"value\":";
                json += choices[i].numeric ? choices[i].value : "\"" + EscapeJson(choices[i].value) + "\"";
                json += '}';
            }
            json += ']';
        }
        if (!options.empty())
        {
            json += ",\"options\":[";
            for (std::size_t i = 0; i < options.size(); ++i)
            {
                if (i)
                    json += ',';
                json += options[i].toJson();
            }
            json += ']';
        }
        return json + '}';
    }

    SlashCommand::SlashCommand(std::string name, std::string description) : name_(std::move(name)), description_(std::move(description)) {}

    CommandOption *SlashCommand::findOptionRecursive(std::vector<CommandOption> &options, CommandOptionHandle handle)
    {
        for (auto &option : options)
        {
            if (option.handle == handle)
                return &option;
            if (auto *found = findOptionRecursive(option.options, handle))
                return found;
        }
        return nullptr;
    }

    CommandOption *SlashCommand::findOption(CommandOptionHandle handle) { return handle ? findOptionRecursive(options_, handle) : nullptr; }

    CommandOptionHandle SlashCommand::addOption(CommandOption option, CommandOptionHandle parent)
    {
        if (nextOption_ == 0)
            nextOption_ = 1;
        option.handle = nextOption_++;
        if (!option.isValid())
            return 0;
        auto *target = &options_;
        if (parent)
        {
            CommandOption *parentOption = findOption(parent);
            if (!parentOption || (parentOption->type != 1 && parentOption->type != 2))
                return 0;
            if (parentOption->type == 2 && option.type != 1)
                return 0;
            if (parentOption->type == 1 && (option.type == 1 || option.type == 2))
                return 0;
            target = &parentOption->options;
        }
        else if (!options_.empty())
        {
            const bool existingSubcommands = options_.front().type == 1 || options_.front().type == 2;
            const bool newSubcommand = option.type == 1 || option.type == 2;
            if (existingSubcommands != newSubcommand)
                return 0;
        }
        if (target->size() >= 25)
            return 0;
        if (std::any_of(target->begin(), target->end(), [&](const CommandOption &current)
                        { return current.name == option.name; }))
            return 0;
        if (option.required && std::any_of(target->begin(), target->end(), [](const CommandOption &current)
                                           { return !current.required; }))
            return 0;
        const auto handle = option.handle;
        target->push_back(std::move(option));
        return handle;
    }

    bool SlashCommand::addChoice(CommandOptionHandle handle, CommandChoice choice)
    {
        CommandOption *option = findOption(handle);
        if (!option || option->choices.size() >= 25 || option->autocomplete || choice.name.empty() || choice.name.size() > 100 || choice.value.empty())
            return false;
        if (option->type != 3 && option->type != 4 && option->type != 10)
            return false;
        option->choices.push_back(std::move(choice));
        return true;
    }

    bool SlashCommand::addChannelType(CommandOptionHandle handle, int type)
    {
        CommandOption *option = findOption(handle);
        if (!option || option->type != 7 || type < 0 || type > 16 || Contains(option->channelTypes, type))
            return false;
        option->channelTypes.push_back(type);
        return true;
    }

    bool SlashCommand::setAutocomplete(CommandOptionHandle handle, bool enabled)
    {
        CommandOption *option = findOption(handle);
        if (!option || (option->type != 3 && option->type != 4 && option->type != 10) || (enabled && !option->choices.empty()))
            return false;
        option->autocomplete = enabled;
        return true;
    }

    bool SlashCommand::setNumberRange(CommandOptionHandle handle, double minValue, double maxValue)
    {
        CommandOption *option = findOption(handle);
        if (!option || (option->type != 4 && option->type != 10) || !std::isfinite(minValue) || !std::isfinite(maxValue) || minValue > maxValue)
            return false;
        option->minValue = minValue;
        option->maxValue = maxValue;
        return true;
    }

    bool SlashCommand::setStringLength(CommandOptionHandle handle, int minLength, int maxLength)
    {
        CommandOption *option = findOption(handle);
        if (!option || option->type != 3 || minLength < 0 || maxLength < 1 || minLength > maxLength || maxLength > 6000)
            return false;
        option->minLength = minLength;
        option->maxLength = maxLength;
        return true;
    }

    void SlashCommand::setDefaultMemberPermissions(std::string permissions) { defaultMemberPermissions_ = std::move(permissions); }
    void SlashCommand::setNsfw(bool enabled) { nsfw_ = enabled; }
    bool SlashCommand::addContext(int context)
    {
        if (context < 0 || context > 2 || Contains(contexts_, context))
            return false;
        contexts_.push_back(context);
        return true;
    }
    bool SlashCommand::addIntegrationType(int type)
    {
        if (type < 0 || type > 1 || Contains(integrationTypes_, type))
            return false;
        integrationTypes_.push_back(type);
        return true;
    }
    bool SlashCommand::isValid() const { return ValidName(name_) && !description_.empty() && description_.size() <= 100 && options_.size() <= 25; }

    std::string SlashCommand::toJson() const
    {
        if (!isValid())
            return {};
        std::string json = "{\"name\":\"" + EscapeJson(name_) + "\",\"description\":\"" + EscapeJson(description_) + "\",\"type\":1";
        if (!defaultMemberPermissions_.empty())
            json += ",\"default_member_permissions\":\"" + EscapeJson(defaultMemberPermissions_) + "\"";
        if (nsfw_)
            json += ",\"nsfw\":true";
        if (!contexts_.empty())
            json += ",\"contexts\":" + IntArray(contexts_);
        if (!integrationTypes_.empty())
            json += ",\"integration_types\":" + IntArray(integrationTypes_);
        if (!options_.empty())
        {
            json += ",\"options\":[";
            for (std::size_t i = 0; i < options_.size(); ++i)
            {
                if (i)
                    json += ',';
                json += options_[i].toJson();
            }
            json += ']';
        }
        return json + '}';
    }

    CommandHandle CommandManager::create(const std::string &name, const std::string &description)
    {
        auto command = std::make_unique<SlashCommand>(name, description);
        if (!command->isValid())
            return 0;
        while (next_ == 0 || commands_.count(next_))
            ++next_;
        const CommandHandle handle = next_++;
        commands_[handle] = std::move(command);
        dirty_ = true;
        return handle;
    }
    bool CommandManager::destroy(CommandHandle handle)
    {
        if (!handle || !commands_.erase(handle))
            return false;
        dirty_ = true;
        return true;
    }
    SlashCommand *CommandManager::get(CommandHandle handle)
    {
        const auto it = commands_.find(handle);
        return it == commands_.end() ? nullptr : it->second.get();
    }
    CommandOptionHandle CommandManager::addOption(CommandHandle handle, CommandOption option, CommandOptionHandle parent)
    {
        auto *command = get(handle);
        if (!command)
            return 0;
        const auto result = command->addOption(std::move(option), parent);
        if (result)
            dirty_ = true;
        return result;
    }
    void CommandManager::clear()
    {
        commands_.clear();
        next_ = 1;
        dirty_ = true;
    }
    void CommandManager::setGuild(std::string guildId, bool autoDeploy)
    {
        guildId_ = std::move(guildId);
        autoDeploy_ = autoDeploy;
        dirty_ = true;
    }
    const std::string &CommandManager::guildId() const { return guildId_; }
    bool CommandManager::autoDeploy() const { return autoDeploy_; }
    bool CommandManager::dirty() const { return dirty_; }
    void CommandManager::markDirty() { dirty_ = true; }
    void CommandManager::markDeployed() { dirty_ = false; }
    std::string CommandManager::toJson() const
    {
        std::vector<CommandHandle> handles;
        handles.reserve(commands_.size());
        for (const auto &[handle, _] : commands_)
            handles.push_back(handle);
        std::sort(handles.begin(), handles.end());
        std::string json = "[";
        bool first = true;
        for (const auto handle : handles)
        {
            const auto it = commands_.find(handle);
            if (it == commands_.end())
                continue;
            const std::string value = it->second->toJson();
            if (value.empty())
                continue;
            if (!first)
                json += ',';
            json += value;
            first = false;
        }
        return json + ']';
    }
}
