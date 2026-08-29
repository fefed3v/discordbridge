#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace DiscordBridge
{
    using V2Handle = std::uint32_t;
    using V2ComponentHandle = std::uint32_t;

    class ComponentsV2
    {
    public:
        bool addText(const std::string &content);
        bool addSeparator(bool divider = true, int spacing = 1);
        bool addContainer(const std::string &content, int accentColor = -1, bool spoiler = false);
        bool addSection(const std::string &content, const std::string &thumbnailUrl);
        bool addSectionButton(const std::string &content, const std::string &label, const std::string &customId, int style = 1, bool disabled = false);
        bool addMedia(const std::string &url, const std::string &description = "", bool spoiler = false);
        bool addActionRow(const std::string &actionRowJson);
        bool addFile(const std::string &attachmentName, bool spoiler = false);
        bool addThumbnail(const std::string &url, const std::string &description = "", bool spoiler = false);

        V2ComponentHandle createContainer(int accentColor = -1, bool spoiler = false);
        bool containerAddText(V2ComponentHandle handle, const std::string &content);
        bool containerAddSeparator(V2ComponentHandle handle, bool divider = true, int spacing = 1);
        bool containerAddActionRow(V2ComponentHandle handle, const std::string &actionRowJson);
        bool containerAddMedia(V2ComponentHandle handle, const std::string &url, const std::string &description = "", bool spoiler = false);

        V2ComponentHandle createSection();
        bool sectionAddText(V2ComponentHandle handle, const std::string &content);
        bool sectionSetThumbnail(V2ComponentHandle handle, const std::string &url, const std::string &description = "", bool spoiler = false);
        bool sectionSetButton(V2ComponentHandle handle, const std::string &label, const std::string &customId, int style = 1, bool disabled = false);

        void clear();
        bool empty() const;
        std::string componentsJson() const;
        std::string messageJson(int extraFlags = 0) const;

    private:
        struct Component
        {
            V2ComponentHandle handle{0};
            int type{0};
            std::string json;
            std::vector<std::string> children;
            std::string accessory;
            int accentColor{-1};
            bool spoiler{false};
        };

        static std::string escapeJson(const std::string &value);
        static std::string textJson(const std::string &content);
        static std::string separatorJson(bool divider, int spacing);
        static std::string mediaGalleryJson(const std::string &url, const std::string &description, bool spoiler);
        static std::string thumbnailJson(const std::string &url, const std::string &description, bool spoiler);
        Component *find(V2ComponentHandle handle);
        std::string render(const Component &component) const;
        bool canAddTopLevel() const;

        std::vector<Component> components_;
        V2ComponentHandle nextComponent_{1};
    };

    class ComponentsV2Manager
    {
    public:
        V2Handle create();
        bool destroy(V2Handle handle);
        ComponentsV2 *get(V2Handle handle);
        const ComponentsV2 *get(V2Handle handle) const;
        void clear();

    private:
        std::unordered_map<V2Handle, std::unique_ptr<ComponentsV2>> messages_;
        V2Handle next_{1};
    };
}
