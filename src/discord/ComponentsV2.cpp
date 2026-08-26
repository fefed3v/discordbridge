#include "ComponentsV2.hpp"

#include <sstream>

namespace DiscordBridge
{
    std::string ComponentsV2::escapeJson(const std::string& value)
    {
        std::string out; out.reserve(value.size());
        for (char c : value)
        {
            switch (c)
            {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default: out += c; break;
            }
        }
        return out;
    }

    bool ComponentsV2::addText(const std::string& content)
    {
        if (content.empty() || components_.size() >= 40) return false;
        components_.push_back("{\"type\":10,\"content\":\"" + escapeJson(content) + "\"}");
        return true;
    }

    bool ComponentsV2::addSeparator(bool divider, int spacing)
    {
        if (components_.size() >= 40 || spacing < 1 || spacing > 2) return false;
        components_.push_back("{\"type\":14,\"divider\":" + std::string(divider ? "true" : "false") + ",\"spacing\":" + std::to_string(spacing) + "}");
        return true;
    }

    bool ComponentsV2::addContainer(const std::string& content, int accentColor, bool spoiler)
    {
        if (content.empty() || components_.size() >= 40 || accentColor > 0xFFFFFF) return false;
        std::string json = "{\"type\":17";
        if (accentColor >= 0) json += ",\"accent_color\":" + std::to_string(accentColor);
        if (spoiler) json += ",\"spoiler\":true";
        json += ",\"components\":[{\"type\":10,\"content\":\"" + escapeJson(content) + "\"}]}";
        components_.push_back(std::move(json));
        return true;
    }

    bool ComponentsV2::addSection(const std::string& content, const std::string& thumbnailUrl)
    {
        if (content.empty() || thumbnailUrl.empty() || components_.size() >= 40) return false;
        components_.push_back("{\"type\":9,\"components\":[{\"type\":10,\"content\":\"" + escapeJson(content) + "\"}],\"accessory\":{\"type\":11,\"media\":{\"url\":\"" + escapeJson(thumbnailUrl) + "\"}}}");
        return true;
    }

    bool ComponentsV2::addSectionButton(const std::string& content, const std::string& label, const std::string& customId, int style, bool disabled)
    {
        if (content.empty() || label.empty() || customId.empty() || components_.size() >= 40 || style < 1 || style > 4) return false;
        std::string button = "{\"type\":2,\"style\":" + std::to_string(style) + ",\"label\":\"" + escapeJson(label) + "\",\"custom_id\":\"" + escapeJson(customId) + "\"";
        if (disabled) button += ",\"disabled\":true";
        button += "}";
        components_.push_back("{\"type\":9,\"components\":[{\"type\":10,\"content\":\"" + escapeJson(content) + "\"}],\"accessory\":" + button + "}");
        return true;
    }

    bool ComponentsV2::addMedia(const std::string& url, const std::string& description, bool spoiler)
    {
        if (url.empty() || components_.size() >= 40) return false;
        std::string json = "{\"type\":12,\"items\":[{\"media\":{\"url\":\"" + escapeJson(url) + "\"}";
        if (!description.empty()) json += ",\"description\":\"" + escapeJson(description) + "\"";
        if (spoiler) json += ",\"spoiler\":true";
        json += "}]}";
        components_.push_back(std::move(json));
        return true;
    }

    bool ComponentsV2::addActionRow(const std::string& actionRowJson)
    {
        if (actionRowJson.empty() || components_.size() >= 40) return false;
        components_.push_back(actionRowJson);
        return true;
    }

    void ComponentsV2::clear() { components_.clear(); }
    bool ComponentsV2::empty() const { return components_.empty(); }

    std::string ComponentsV2::componentsJson() const
    {
        std::ostringstream out; out << '[';
        for (std::size_t i=0;i<components_.size();++i) { if(i) out << ','; out << components_[i]; }
        out << ']'; return out.str();
    }

    std::string ComponentsV2::messageJson(int extraFlags) const
    {
        return "{\"flags\":" + std::to_string(32768 | extraFlags) + ",\"components\":" + componentsJson() + "}";
    }

    V2Handle ComponentsV2Manager::create()
    {
        for (std::size_t i=0;i<0xFFFFFFFFu;++i) { V2Handle h=next_++; if(h==0) continue; if(!messages_.contains(h)) { messages_[h]=std::make_unique<ComponentsV2>(); return h; } }
        return 0;
    }
    bool ComponentsV2Manager::destroy(V2Handle h) { return messages_.erase(h)>0; }
    ComponentsV2* ComponentsV2Manager::get(V2Handle h) { auto it=messages_.find(h); return it==messages_.end()?nullptr:it->second.get(); }
    const ComponentsV2* ComponentsV2Manager::get(V2Handle h) const { auto it=messages_.find(h); return it==messages_.end()?nullptr:it->second.get(); }
    void ComponentsV2Manager::clear() { messages_.clear(); next_=1; }
}
