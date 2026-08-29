#include "SelectMenu.hpp"

#include <algorithm>

namespace DiscordBridge
{
    namespace
    {
        std::string Escape(const std::string &value)
        {
            std::string out;
            for (const char c : value)
            {
                if (c == '"' || c == '\\')
                    out += '\\';
                if (c == '\n')
                {
                    out += "\\n";
                    continue;
                }
                out += c;
            }
            return out;
        }
    }

    bool SelectMenu::addOption(const SelectOption &option)
    {
        if (type_ != 3 || options_.size() >= 25 || option.label.empty() || option.label.size() > 100 || option.value.empty() || option.value.size() > 100 || option.description.size() > 100)
            return false;
        options_.push_back(option);
        return true;
    }
    bool SelectMenu::addChannelType(int type)
    {
        if (type_ != 8 || type < 0 || type > 16 || std::find(channelTypes_.begin(), channelTypes_.end(), type) != channelTypes_.end())
            return false;
        channelTypes_.push_back(type);
        return true;
    }
    bool SelectMenu::isValid() const
    {
        if (type_ < 3 || type_ > 8 || type_ == 4 || customId_.empty() || customId_.size() > 100 || placeholder_.size() > 150 || minValues_ < 0 || maxValues_ < 1 || minValues_ > maxValues_ || maxValues_ > 25)
            return false;
        return type_ != 3 || (!options_.empty() && maxValues_ <= static_cast<int>(options_.size()));
    }
    std::string SelectMenu::toJson() const
    {
        if (!isValid())
            return {};
        std::string json = "{\"type\":" + std::to_string(type_) + ",\"custom_id\":\"" + Escape(customId_) + "\"";
        if (type_ == 3)
        {
            json += ",\"options\":[";
            for (std::size_t i = 0; i < options_.size(); ++i)
            {
                const auto &option = options_[i];
                if (i)
                    json += ',';
                json += "{\"label\":\"" + Escape(option.label) + "\",\"value\":\"" + Escape(option.value) + "\"";
                if (!option.description.empty())
                    json += ",\"description\":\"" + Escape(option.description) + "\"";
                if (!option.emoji.empty())
                    json += ",\"emoji\":{\"name\":\"" + Escape(option.emoji) + "\"}";
                if (option.isDefault)
                    json += ",\"default\":true";
                json += '}';
            }
            json += ']';
        }
        if (type_ == 8 && !channelTypes_.empty())
        {
            json += ",\"channel_types\":[";
            for (std::size_t i = 0; i < channelTypes_.size(); ++i)
            {
                if (i)
                    json += ',';
                json += std::to_string(channelTypes_[i]);
            }
            json += ']';
        }
        if (!placeholder_.empty())
            json += ",\"placeholder\":\"" + Escape(placeholder_) + "\"";
        json += ",\"min_values\":" + std::to_string(minValues_) + ",\"max_values\":" + std::to_string(maxValues_);
        if (disabled_)
            json += ",\"disabled\":true";
        return json + '}';
    }
    SelectMenuHandle SelectMenuManager::create()
    {
        while (next_ == 0 || items_.count(next_))
            ++next_;
        const auto handle = next_++;
        items_[handle] = std::make_unique<SelectMenu>();
        return handle;
    }
    bool SelectMenuManager::destroy(SelectMenuHandle handle) { return handle && items_.erase(handle) > 0; }
    SelectMenu *SelectMenuManager::get(SelectMenuHandle handle)
    {
        const auto it = items_.find(handle);
        return it == items_.end() ? nullptr : it->second.get();
    }
    const SelectMenu *SelectMenuManager::get(SelectMenuHandle handle) const
    {
        const auto it = items_.find(handle);
        return it == items_.end() ? nullptr : it->second.get();
    }
}
