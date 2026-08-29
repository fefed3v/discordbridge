#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "SelectMenu.hpp"

namespace DiscordBridge
{
    using ButtonHandle = std::uint32_t;
    using ActionRowHandle = std::uint32_t;

    enum class ButtonStyle : int
    {
        Primary = 1,
        Secondary = 2,
        Success = 3,
        Danger = 4,
        Link = 5,
        Premium = 6
    };

    class Button
    {
    public:
        void setLabel(const std::string& label);
        void setCustomId(const std::string& customId);
        void setUrl(const std::string& url);
        void setEmoji(const std::string& emoji);
        void setEmojiEx(const std::string& name, const std::string& id, bool animated);
        void setSkuId(const std::string& skuId);
        void setComponentId(std::uint32_t componentId);
        void setStyle(ButtonStyle style);
        void setDisabled(bool disabled);

        void clear();

        bool empty() const;
        bool isValid() const;

        const std::string& getLabel() const;
        const std::string& getCustomId() const;
        const std::string& getUrl() const;
        const std::string& getEmoji() const;
        const std::string& getEmojiId() const;
        const std::string& getSkuId() const;
        std::uint32_t getComponentId() const;
        bool isEmojiAnimated() const;

        ButtonStyle getStyle() const;
        bool isDisabled() const;

        std::string toJson() const;

    private:
        std::string label_;
        std::string customId_;
        std::string url_;
        std::string emoji_;
        std::string emojiId_;
        std::string skuId_;
        std::uint32_t componentId_{0};
        bool emojiAnimated_{false};

        ButtonStyle style_{ButtonStyle::Primary};
        bool disabled_{false};
    };

    class ButtonManager
    {
    public:
        ButtonHandle create();
        bool destroy(ButtonHandle handle);

        Button* get(ButtonHandle handle);
        const Button* get(ButtonHandle handle) const;

        void clear();
        std::size_t size() const;

    private:
        std::unordered_map<ButtonHandle, std::unique_ptr<Button>> buttons_;
        ButtonHandle nextHandle_{1};
    };

    class ActionRow
    {
    public:
        bool addButton(ButtonHandle handle);
        bool setSelectMenu(SelectMenuHandle handle);
        void clearSelectMenu();
        bool removeButton(ButtonHandle handle);
        bool hasButton(ButtonHandle handle) const;

        void clear();

        bool empty() const;
        std::size_t size() const;

        const std::vector<ButtonHandle>& getButtons() const;

        std::string toJson(const ButtonManager& buttonManager, const SelectMenuManager* selectManager = nullptr) const;

    private:
        std::vector<ButtonHandle> buttons_;
        SelectMenuHandle selectMenu_{0};
    };

    class ActionRowManager
    {
    public:
        ActionRowHandle create();
        bool destroy(ActionRowHandle handle);

        ActionRow* get(ActionRowHandle handle);
        const ActionRow* get(ActionRowHandle handle) const;

        void clear();
        std::size_t size() const;

    private:
        std::unordered_map<ActionRowHandle, std::unique_ptr<ActionRow>> rows_;
        ActionRowHandle nextHandle_{1};
    };
}