#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace DiscordBridge
{
    using SelectMenuHandle = std::uint32_t;
    struct SelectOption
    {
        std::string label, value, description, emoji;
        bool isDefault{false};
    };

    class SelectMenu
    {
    public:
        void setType(int value) { type_ = value; }
        void setCustomId(const std::string &value) { customId_ = value; }
        void setPlaceholder(const std::string &value) { placeholder_ = value; }
        void setMinValues(int value) { minValues_ = value; }
        void setMaxValues(int value) { maxValues_ = value; }
        void setDisabled(bool value) { disabled_ = value; }
        bool addOption(const SelectOption &option);
        bool addChannelType(int type);
        void clearOptions() { options_.clear(); }
        bool isValid() const;
        std::string toJson() const;

    private:
        int type_{3};
        std::string customId_, placeholder_;
        int minValues_{1}, maxValues_{1};
        bool disabled_{false};
        std::vector<SelectOption> options_;
        std::vector<int> channelTypes_;
    };

    class SelectMenuManager
    {
    public:
        SelectMenuHandle create();
        bool destroy(SelectMenuHandle handle);
        SelectMenu *get(SelectMenuHandle handle);
        const SelectMenu *get(SelectMenuHandle handle) const;
        void clear()
        {
            items_.clear();
            next_ = 1;
        }

    private:
        std::unordered_map<SelectMenuHandle, std::unique_ptr<SelectMenu>> items_;
        SelectMenuHandle next_{1};
    };
}
