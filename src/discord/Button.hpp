#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

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
        Link = 5
    };

    class Button
    {
    public:
        void setLabel(const std::string& label);
        void setCustomId(const std::string& customId);
        void setUrl(const std::string& url);
        void setEmoji(const std::string& emoji);
        void setStyle(ButtonStyle style);
        void setDisabled(bool disabled);

        void clear();

        bool empty() const;
        bool isValid() const;

        const std::string& getLabel() const;
        const std::string& getCustomId() const;
        const std::string& getUrl() const;
        const std::string& getEmoji() const;

        ButtonStyle getStyle() const;
        bool isDisabled() const;

        std::string toJson() const;

    private:
        std::string label_;
        std::string customId_;
        std::string url_;
        std::string emoji_;

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
        bool removeButton(ButtonHandle handle);
        bool hasButton(ButtonHandle handle) const;

        void clear();

        bool empty() const;
        std::size_t size() const;

        const std::vector<ButtonHandle>& getButtons() const;

        std::string toJson(const ButtonManager& buttonManager) const;

    private:
        std::vector<ButtonHandle> buttons_;
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