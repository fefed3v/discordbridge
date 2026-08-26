#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace DiscordBridge
{
    using V2Handle = std::uint32_t;

    class ComponentsV2
    {
    public:
        bool addText(const std::string& content);
        bool addSeparator(bool divider = true, int spacing = 1);
        bool addContainer(const std::string& content, int accentColor = -1, bool spoiler = false);
        bool addSection(const std::string& content, const std::string& thumbnailUrl);
        bool addSectionButton(const std::string& content, const std::string& label, const std::string& customId, int style = 1, bool disabled = false);
        bool addMedia(const std::string& url, const std::string& description = "", bool spoiler = false);
        bool addActionRow(const std::string& actionRowJson);
        void clear();
        bool empty() const;
        std::string componentsJson() const;
        std::string messageJson(int extraFlags = 0) const;

    private:
        static std::string escapeJson(const std::string& value);
        std::vector<std::string> components_;
    };

    class ComponentsV2Manager
    {
    public:
        V2Handle create();
        bool destroy(V2Handle handle);
        ComponentsV2* get(V2Handle handle);
        const ComponentsV2* get(V2Handle handle) const;
        void clear();

    private:
        std::unordered_map<V2Handle, std::unique_ptr<ComponentsV2>> messages_;
        V2Handle next_{1};
    };
}
