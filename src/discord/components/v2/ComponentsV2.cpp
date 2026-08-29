#include "ComponentsV2.hpp"

#include <sstream>
#include <utility>

namespace DiscordBridge
{
    std::string ComponentsV2::escapeJson(const std::string &value)
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

    std::string ComponentsV2::textJson(const std::string &content) { return "{\"type\":10,\"content\":\"" + escapeJson(content) + "\"}"; }
    std::string ComponentsV2::separatorJson(bool divider, int spacing) { return "{\"type\":14,\"divider\":" + std::string(divider ? "true" : "false") + ",\"spacing\":" + std::to_string(spacing) + "}"; }
    std::string ComponentsV2::thumbnailJson(const std::string &url, const std::string &description, bool spoiler)
    {
        std::string json = "{\"type\":11,\"media\":{\"url\":\"" + escapeJson(url) + "\"}";
        if (!description.empty())
            json += ",\"description\":\"" + escapeJson(description) + "\"";
        if (spoiler)
            json += ",\"spoiler\":true";
        return json + '}';
    }
    std::string ComponentsV2::mediaGalleryJson(const std::string &url, const std::string &description, bool spoiler)
    {
        std::string json = "{\"type\":12,\"items\":[{\"media\":{\"url\":\"" + escapeJson(url) + "\"}";
        if (!description.empty())
            json += ",\"description\":\"" + escapeJson(description) + "\"";
        if (spoiler)
            json += ",\"spoiler\":true";
        return json + "}]}";
    }

    bool ComponentsV2::canAddTopLevel() const { return components_.size() < 40; }
    ComponentsV2::Component *ComponentsV2::find(V2ComponentHandle handle)
    {
        for (auto &component : components_)
            if (component.handle == handle)
                return &component;
        return nullptr;
    }

    bool ComponentsV2::addText(const std::string &content)
    {
        if (content.empty() || content.size() > 4000 || !canAddTopLevel())
            return false;
        components_.push_back({0, 10, textJson(content), {}, {}, -1, false});
        return true;
    }
    bool ComponentsV2::addSeparator(bool divider, int spacing)
    {
        if (!canAddTopLevel() || spacing < 1 || spacing > 2)
            return false;
        components_.push_back({0, 14, separatorJson(divider, spacing), {}, {}, -1, false});
        return true;
    }
    bool ComponentsV2::addContainer(const std::string &content, int accentColor, bool spoiler)
    {
        const auto handle = createContainer(accentColor, spoiler);
        return handle && containerAddText(handle, content);
    }
    bool ComponentsV2::addSection(const std::string &content, const std::string &thumbnailUrl)
    {
        const auto handle = createSection();
        return handle && sectionAddText(handle, content) && sectionSetThumbnail(handle, thumbnailUrl);
    }
    bool ComponentsV2::addSectionButton(const std::string &content, const std::string &label, const std::string &customId, int style, bool disabled)
    {
        const auto handle = createSection();
        return handle && sectionAddText(handle, content) && sectionSetButton(handle, label, customId, style, disabled);
    }
    bool ComponentsV2::addMedia(const std::string &url, const std::string &description, bool spoiler)
    {
        if (url.empty() || !canAddTopLevel())
            return false;
        components_.push_back({0, 12, mediaGalleryJson(url, description, spoiler), {}, {}, -1, false});
        return true;
    }
    bool ComponentsV2::addActionRow(const std::string &actionRowJson)
    {
        if (actionRowJson.empty() || !canAddTopLevel())
            return false;
        components_.push_back({0, 1, actionRowJson, {}, {}, -1, false});
        return true;
    }
    bool ComponentsV2::addFile(const std::string &attachmentName, bool spoiler)
    {
        if (attachmentName.empty() || !canAddTopLevel())
            return false;
        std::string json = "{\"type\":13,\"file\":{\"url\":\"attachment://" + escapeJson(attachmentName) + "\"}";
        if (spoiler)
            json += ",\"spoiler\":true";
        components_.push_back({0, 13, json + '}', {}, {}, -1, false});
        return true;
    }
    bool ComponentsV2::addThumbnail(const std::string &url, const std::string &description, bool spoiler)
    {
        if (url.empty() || !canAddTopLevel())
            return false;
        components_.push_back({0, 11, thumbnailJson(url, description, spoiler), {}, {}, -1, false});
        return true;
    }

    V2ComponentHandle ComponentsV2::createContainer(int accentColor, bool spoiler)
    {
        if (!canAddTopLevel() || accentColor > 0xFFFFFF)
            return 0;
        while (nextComponent_ == 0)
            ++nextComponent_;
        const auto handle = nextComponent_++;
        Component component;
        component.handle = handle;
        component.type = 17;
        component.accentColor = accentColor;
        component.spoiler = spoiler;
        components_.push_back(std::move(component));
        return handle;
    }
    bool ComponentsV2::containerAddText(V2ComponentHandle handle, const std::string &content)
    {
        auto *c = find(handle);
        if (!c || c->type != 17 || content.empty() || content.size() > 4000 || c->children.size() >= 40)
            return false;
        c->children.push_back(textJson(content));
        return true;
    }
    bool ComponentsV2::containerAddSeparator(V2ComponentHandle handle, bool divider, int spacing)
    {
        auto *c = find(handle);
        if (!c || c->type != 17 || spacing < 1 || spacing > 2 || c->children.size() >= 40)
            return false;
        c->children.push_back(separatorJson(divider, spacing));
        return true;
    }
    bool ComponentsV2::containerAddActionRow(V2ComponentHandle handle, const std::string &actionRowJson)
    {
        auto *c = find(handle);
        if (!c || c->type != 17 || actionRowJson.empty() || c->children.size() >= 40)
            return false;
        c->children.push_back(actionRowJson);
        return true;
    }
    bool ComponentsV2::containerAddMedia(V2ComponentHandle handle, const std::string &url, const std::string &description, bool spoiler)
    {
        auto *c = find(handle);
        if (!c || c->type != 17 || url.empty() || c->children.size() >= 40)
            return false;
        c->children.push_back(mediaGalleryJson(url, description, spoiler));
        return true;
    }

    V2ComponentHandle ComponentsV2::createSection()
    {
        if (!canAddTopLevel())
            return 0;
        while (nextComponent_ == 0)
            ++nextComponent_;
        const auto handle = nextComponent_++;
        Component component;
        component.handle = handle;
        component.type = 9;
        components_.push_back(std::move(component));
        return handle;
    }
    bool ComponentsV2::sectionAddText(V2ComponentHandle handle, const std::string &content)
    {
        auto *c = find(handle);
        if (!c || c->type != 9 || content.empty() || content.size() > 4000 || c->children.size() >= 3)
            return false;
        c->children.push_back(textJson(content));
        return true;
    }
    bool ComponentsV2::sectionSetThumbnail(V2ComponentHandle handle, const std::string &url, const std::string &description, bool spoiler)
    {
        auto *c = find(handle);
        if (!c || c->type != 9 || url.empty())
            return false;
        c->accessory = thumbnailJson(url, description, spoiler);
        return true;
    }
    bool ComponentsV2::sectionSetButton(V2ComponentHandle handle, const std::string &label, const std::string &customId, int style, bool disabled)
    {
        auto *c = find(handle);
        if (!c || c->type != 9 || label.empty() || label.size() > 80 || customId.empty() || customId.size() > 100 || style < 1 || style > 4)
            return false;
        c->accessory = "{\"type\":2,\"style\":" + std::to_string(style) + ",\"label\":\"" + escapeJson(label) + "\",\"custom_id\":\"" + escapeJson(customId) + "\",\"disabled\":" + (disabled ? "true" : "false") + "}";
        return true;
    }

    std::string ComponentsV2::render(const Component &component) const
    {
        if (!component.json.empty())
            return component.json;
        if (component.type == 17)
        {
            std::string json = "{\"type\":17";
            if (component.accentColor >= 0)
                json += ",\"accent_color\":" + std::to_string(component.accentColor);
            if (component.spoiler)
                json += ",\"spoiler\":true";
            json += ",\"components\":[";
            for (std::size_t i = 0; i < component.children.size(); ++i)
            {
                if (i)
                    json += ',';
                json += component.children[i];
            }
            return json + "]}";
        }
        if (component.type == 9)
        {
            if (component.children.empty() || component.accessory.empty())
                return {};
            std::string json = "{\"type\":9,\"components\":[";
            for (std::size_t i = 0; i < component.children.size(); ++i)
            {
                if (i)
                    json += ',';
                json += component.children[i];
            }
            return json + "],\"accessory\":" + component.accessory + '}';
        }
        return {};
    }

    void ComponentsV2::clear()
    {
        components_.clear();
        nextComponent_ = 1;
    }
    bool ComponentsV2::empty() const { return components_.empty(); }
    std::string ComponentsV2::componentsJson() const
    {
        std::string json = "[";
        bool first = true;
        for (const auto &component : components_)
        {
            const auto rendered = render(component);
            if (rendered.empty())
                continue;
            if (!first)
                json += ',';
            json += rendered;
            first = false;
        }
        return json + ']';
    }
    std::string ComponentsV2::messageJson(int extraFlags) const { return "{\"flags\":" + std::to_string(32768 | extraFlags) + ",\"components\":" + componentsJson() + "}"; }

    V2Handle ComponentsV2Manager::create()
    {
        while (next_ == 0 || messages_.count(next_))
            ++next_;
        const auto handle = next_++;
        messages_[handle] = std::make_unique<ComponentsV2>();
        return handle;
    }
    bool ComponentsV2Manager::destroy(V2Handle handle) { return handle && messages_.erase(handle) > 0; }
    ComponentsV2 *ComponentsV2Manager::get(V2Handle handle)
    {
        const auto it = messages_.find(handle);
        return it == messages_.end() ? nullptr : it->second.get();
    }
    const ComponentsV2 *ComponentsV2Manager::get(V2Handle handle) const
    {
        const auto it = messages_.find(handle);
        return it == messages_.end() ? nullptr : it->second.get();
    }
    void ComponentsV2Manager::clear()
    {
        messages_.clear();
        next_ = 1;
    }
}
