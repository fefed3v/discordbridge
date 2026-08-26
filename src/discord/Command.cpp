#include "Command.hpp"

#include <algorithm>
#include <cctype>

namespace DiscordBridge
{
    namespace
    {
        std::string EscapeJson(const std::string& value)
        {
            std::string result;
            result.reserve(value.size());
            for (char c : value)
            {
                switch (c)
                {
                    case '"': result += "\\\""; break;
                    case '\\': result += "\\\\"; break;
                    case '\n': result += "\\n"; break;
                    case '\r': result += "\\r"; break;
                    case '\t': result += "\\t"; break;
                    default: result.push_back(c); break;
                }
            }
            return result;
        }

        bool ValidName(const std::string& value)
        {
            if (value.empty() || value.size() > 32) return false;
            for (const unsigned char c : value)
            {
                if (!(std::islower(c) || std::isdigit(c) || c == '-' || c == '_')) return false;
            }
            return true;
        }
    }

    SlashCommand::SlashCommand(std::string name, std::string description) : name_(std::move(name)), description_(std::move(description)) {}

    bool SlashCommand::addOption(const CommandOption& option)
    {
        if (options_.size() >= 25 || !ValidName(option.name) || option.description.empty() || option.description.size() > 100) return false;
        if (option.type < 3 || option.type > 11) return false;
        if (std::any_of(options_.begin(), options_.end(), [&](const CommandOption& item) { return item.name == option.name; })) return false;
        if (option.required && std::any_of(options_.begin(), options_.end(), [](const CommandOption& item) { return !item.required; })) return false;
        options_.push_back(option);
        return true;
    }

    bool SlashCommand::isValid() const
    {
        return ValidName(name_) && !description_.empty() && description_.size() <= 100;
    }

    std::string SlashCommand::toJson() const
    {
        if (!isValid()) return {};
        std::string json = "{\"name\":\"" + EscapeJson(name_) + "\",\"description\":\"" + EscapeJson(description_) + "\",\"type\":1";
        if (!options_.empty())
        {
            json += ",\"options\":[";
            for (std::size_t i = 0; i < options_.size(); ++i)
            {
                if (i) json += ',';
                const auto& option = options_[i];
                json += "{\"type\":" + std::to_string(option.type) + ",\"name\":\"" + EscapeJson(option.name) + "\",\"description\":\"" + EscapeJson(option.description) + "\",\"required\":" + (option.required ? "true" : "false") + "}";
            }
            json += ']';
        }
        json += '}';
        return json;
    }

    CommandHandle CommandManager::create(const std::string& name, const std::string& description)
    {
        auto command = std::make_unique<SlashCommand>(name, description);
        if (!command->isValid()) return 0;
        while (next_ == 0 || commands_.count(next_)) ++next_;
        const CommandHandle handle = next_++;
        commands_[handle] = std::move(command);
        dirty_ = true;
        return handle;
    }

    bool CommandManager::destroy(CommandHandle handle)
    {
        if (!handle || !commands_.erase(handle)) return false;
        dirty_ = true;
        return true;
    }

    SlashCommand* CommandManager::get(CommandHandle handle)
    {
        const auto it = commands_.find(handle);
        return it == commands_.end() ? nullptr : it->second.get();
    }

    bool CommandManager::addOption(CommandHandle handle, const CommandOption& option)
    {
        SlashCommand* command = get(handle);
        if (!command || !command->addOption(option)) return false;
        dirty_ = true;
        return true;
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

    const std::string& CommandManager::guildId() const { return guildId_; }
    bool CommandManager::autoDeploy() const { return autoDeploy_; }
    bool CommandManager::dirty() const { return dirty_; }
    void CommandManager::markDirty() { dirty_ = true; }
    void CommandManager::markDeployed() { dirty_ = false; }

    std::string CommandManager::toJson() const
    {
        std::vector<CommandHandle> handles;
        handles.reserve(commands_.size());
        for (const auto& entry : commands_) handles.push_back(entry.first);
        std::sort(handles.begin(), handles.end());

        std::string json = "[";
        bool first = true;
        for (const CommandHandle handle : handles)
        {
            const auto it = commands_.find(handle);
            if (it == commands_.end()) continue;
            const std::string command = it->second->toJson();
            if (command.empty()) continue;
            if (!first) json += ',';
            json += command;
            first = false;
        }
        json += ']';
        return json;
    }
}
