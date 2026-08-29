#include "Button.hpp"

#include <algorithm>

namespace DiscordBridge
{
    namespace
    {
        std::string EscapeJson(const std::string& value)
        {
            std::string result;
            result.reserve(value.size());

            for (const char character : value)
            {
                switch (character)
                {
                    case '"': result += "\\\""; break;
                    case '\\': result += "\\\\"; break;
                    case '\n': result += "\\n"; break;
                    case '\r': result += "\\r"; break;
                    case '\t': result += "\\t"; break;
                    case '\b': result += "\\b"; break;
                    case '\f': result += "\\f"; break;
                    default: result.push_back(character); break;
                }
            }

            return result;
        }
    }

    void Button::setLabel(const std::string& label)
    {
        label_ = label;
    }

    void Button::setCustomId(const std::string& customId)
    {
        customId_ = customId;
    }

    void Button::setUrl(const std::string& url)
    {
        url_ = url;
    }

    void Button::setEmoji(const std::string& emoji)
    {
        emoji_ = emoji;
        emojiId_.clear();
        emojiAnimated_ = false;
    }

    void Button::setEmojiEx(const std::string& name, const std::string& id, bool animated)
    {
        emoji_ = name;
        emojiId_ = id;
        emojiAnimated_ = animated;
    }

    void Button::setSkuId(const std::string& skuId)
    {
        skuId_ = skuId;
    }

    void Button::setComponentId(std::uint32_t componentId)
    {
        componentId_ = componentId;
    }

    void Button::setStyle(ButtonStyle style)
    {
        style_ = style;
    }

    void Button::setDisabled(bool disabled)
    {
        disabled_ = disabled;
    }

    void Button::clear()
    {
        label_.clear();
        customId_.clear();
        url_.clear();
        emoji_.clear();
        emojiId_.clear();
        skuId_.clear();
        componentId_ = 0;
        emojiAnimated_ = false;
        style_ = ButtonStyle::Primary;
        disabled_ = false;
    }

    bool Button::empty() const
    {
        return label_.empty() && customId_.empty() && url_.empty() && emoji_.empty() && emojiId_.empty() && skuId_.empty() && componentId_ == 0;
    }

    bool Button::isValid() const
    {
        if (static_cast<int>(style_) < 1 || static_cast<int>(style_) > 6) return false;
        if (label_.size() > 80 || customId_.size() > 100 || url_.size() > 512) return false;
        if (!emojiId_.empty() && emoji_.empty()) return false;

        if (style_ == ButtonStyle::Premium)
        {
            return !skuId_.empty() && customId_.empty() && label_.empty() && url_.empty() && emoji_.empty() && emojiId_.empty();
        }

        if (style_ == ButtonStyle::Link)
        {
            return !url_.empty() && customId_.empty() && skuId_.empty() && (!label_.empty() || !emoji_.empty());
        }

        return !customId_.empty() && url_.empty() && skuId_.empty() && (!label_.empty() || !emoji_.empty());
    }

    const std::string& Button::getLabel() const
    {
        return label_;
    }

    const std::string& Button::getCustomId() const
    {
        return customId_;
    }

    const std::string& Button::getUrl() const
    {
        return url_;
    }

    const std::string& Button::getEmoji() const
    {
        return emoji_;
    }

    const std::string& Button::getEmojiId() const
    {
        return emojiId_;
    }

    const std::string& Button::getSkuId() const
    {
        return skuId_;
    }

    std::uint32_t Button::getComponentId() const
    {
        return componentId_;
    }

    bool Button::isEmojiAnimated() const
    {
        return emojiAnimated_;
    }

    ButtonStyle Button::getStyle() const
    {
        return style_;
    }

    bool Button::isDisabled() const
    {
        return disabled_;
    }

    std::string Button::toJson() const
    {
        if (!isValid()) return {};

        std::string json = "{\"type\":2,\"style\":" + std::to_string(static_cast<int>(style_));

        if (componentId_ != 0) json += ",\"id\":" + std::to_string(componentId_);
        if (!label_.empty()) json += ",\"label\":\"" + EscapeJson(label_) + "\"";

        if (style_ == ButtonStyle::Link) json += ",\"url\":\"" + EscapeJson(url_) + "\"";
        else if (style_ == ButtonStyle::Premium) json += ",\"sku_id\":\"" + EscapeJson(skuId_) + "\"";
        else json += ",\"custom_id\":\"" + EscapeJson(customId_) + "\"";

        if (!emoji_.empty())
        {
            json += ",\"emoji\":{";
            bool comma = false;
            if (!emojiId_.empty()) { json += "\"id\":\"" + EscapeJson(emojiId_) + "\""; comma = true; }
            if (!emoji_.empty()) { if (comma) json += ','; json += "\"name\":\"" + EscapeJson(emoji_) + "\""; comma = true; }
            if (emojiAnimated_) { if (comma) json += ','; json += "\"animated\":true"; }
            json += '}';
        }

        if (disabled_) json += ",\"disabled\":true";
        json += '}';
        return json;
    }

    ButtonHandle ButtonManager::create()
    {
        if (nextHandle_ == 0) nextHandle_ = 1;

        const ButtonHandle start = nextHandle_;
        ButtonHandle handle = start;

        do
        {
            if (buttons_.find(handle) == buttons_.end())
            {
                buttons_.emplace(handle, std::make_unique<Button>());

                nextHandle_ = handle + 1;
                if (nextHandle_ == 0) nextHandle_ = 1;

                return handle;
            }

            ++handle;

            if (handle == 0) handle = 1;
        }
        while (handle != start);

        return 0;
    }

    bool ButtonManager::destroy(ButtonHandle handle)
    {
        return handle != 0 && buttons_.erase(handle) != 0;
    }

    Button* ButtonManager::get(ButtonHandle handle)
    {
        const auto iterator = buttons_.find(handle);
        return iterator != buttons_.end() ? iterator->second.get() : nullptr;
    }

    const Button* ButtonManager::get(ButtonHandle handle) const
    {
        const auto iterator = buttons_.find(handle);
        return iterator != buttons_.end() ? iterator->second.get() : nullptr;
    }

    void ButtonManager::clear()
    {
        buttons_.clear();
        nextHandle_ = 1;
    }

    std::size_t ButtonManager::size() const
    {
        return buttons_.size();
    }

    bool ActionRow::addButton(ButtonHandle handle)
    {
        if (handle == 0 || selectMenu_ != 0 || buttons_.size() >= 5 || hasButton(handle)) return false;

        buttons_.push_back(handle);
        return true;
    }

    bool ActionRow::setSelectMenu(SelectMenuHandle handle)
    {
        if (handle == 0 || !buttons_.empty()) return false;
        selectMenu_ = handle; return true;
    }

    void ActionRow::clearSelectMenu() { selectMenu_ = 0; }

    bool ActionRow::removeButton(ButtonHandle handle)
    {
        const auto iterator = std::find(buttons_.begin(), buttons_.end(), handle);

        if (iterator == buttons_.end()) return false;

        buttons_.erase(iterator);
        return true;
    }

    bool ActionRow::hasButton(ButtonHandle handle) const
    {
        return std::find(buttons_.begin(), buttons_.end(), handle) != buttons_.end();
    }

    void ActionRow::clear()
    {
        buttons_.clear();
        selectMenu_ = 0;
    }

    bool ActionRow::empty() const
    {
        return buttons_.empty() && selectMenu_ == 0;
    }

    std::size_t ActionRow::size() const
    {
        return buttons_.size();
    }

    const std::vector<ButtonHandle>& ActionRow::getButtons() const
    {
        return buttons_;
    }

    std::string ActionRow::toJson(const ButtonManager& buttonManager, const SelectMenuManager* selectManager) const
    {
        if (selectMenu_ != 0) {
            if (!selectManager) return {};
            const SelectMenu* menu = selectManager->get(selectMenu_);
            if (!menu || !menu->isValid()) return {};
            return "{\"type\":1,\"components\":[" + menu->toJson() + "]}";
        }
        if (buttons_.empty()) return {};

        std::string json = "{\"type\":1,\"components\":[";

        for (std::size_t index = 0; index < buttons_.size(); ++index)
        {
            const Button* button = buttonManager.get(buttons_[index]);

            if (!button || !button->isValid()) return {};

            const std::string buttonJson = button->toJson();
            if (buttonJson.empty()) return {};

            if (index > 0) json += ',';

            json += buttonJson;
        }

        json += "]}";

        return json;
    }

    ActionRowHandle ActionRowManager::create()
    {
        if (nextHandle_ == 0) nextHandle_ = 1;

        const ActionRowHandle start = nextHandle_;
        ActionRowHandle handle = start;

        do
        {
            if (rows_.find(handle) == rows_.end())
            {
                rows_.emplace(handle, std::make_unique<ActionRow>());

                nextHandle_ = handle + 1;
                if (nextHandle_ == 0) nextHandle_ = 1;

                return handle;
            }

            ++handle;

            if (handle == 0) handle = 1;
        }
        while (handle != start);

        return 0;
    }

    bool ActionRowManager::destroy(ActionRowHandle handle)
    {
        return handle != 0 && rows_.erase(handle) != 0;
    }

    ActionRow* ActionRowManager::get(ActionRowHandle handle)
    {
        const auto iterator = rows_.find(handle);
        return iterator != rows_.end() ? iterator->second.get() : nullptr;
    }

    const ActionRow* ActionRowManager::get(ActionRowHandle handle) const
    {
        const auto iterator = rows_.find(handle);
        return iterator != rows_.end() ? iterator->second.get() : nullptr;
    }

    void ActionRowManager::clear()
    {
        rows_.clear();
        nextHandle_ = 1;
    }

    std::size_t ActionRowManager::size() const
    {
        return rows_.size();
    }
}