#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace DiscordBridge
{
    struct EmbedAuthor
    {
        std::string name;
        std::string url;
        std::string iconUrl;

        bool empty() const;
        void clear();
    };

    struct EmbedFooter
    {
        std::string text;
        std::string iconUrl;

        bool empty() const;
        void clear();
    };

    struct EmbedField
    {
        std::string name;
        std::string value;
        bool inlineField{false};
    };

    class Embed final
    {
    private:
        std::string title_;
        std::string description_;
        std::string url_;
        std::string thumbnailUrl_;
        std::string imageUrl_;

        std::uint32_t color_{0};

        EmbedAuthor author_;
        EmbedFooter footer_;

        std::vector<EmbedField> fields_;

    public:
        Embed() = default;

        void setTitle(const std::string& title);
        void setDescription(const std::string& description);
        void setUrl(const std::string& url);
        void setColor(std::uint32_t color);

        void setAuthor(const std::string& name, const std::string& url = "", const std::string& iconUrl = "");
        void clearAuthor();

        void setThumbnail(const std::string& url);
        void clearThumbnail();

        void setImage(const std::string& url);
        void clearImage();

        void setFooter(const std::string& text, const std::string& iconUrl = "");
        void clearFooter();

        bool addField(const std::string& name, const std::string& value, bool inlineField = false);
        void clearFields();

        void clear();

        bool empty() const;

        const std::string& getTitle() const;
        const std::string& getDescription() const;
        const std::string& getUrl() const;
        std::uint32_t getColor() const;

        const EmbedAuthor& getAuthor() const;
        const std::string& getThumbnail() const;
        const std::string& getImage() const;
        const EmbedFooter& getFooter() const;
        const std::vector<EmbedField>& getFields() const;

        std::string toJson() const;
    };

    using EmbedHandle = std::uint32_t;

    class EmbedManager final
    {
    private:
        std::unordered_map<EmbedHandle, std::unique_ptr<Embed>> embeds_;
        EmbedHandle nextHandle_{1};

    public:
        EmbedHandle create();
        bool destroy(EmbedHandle handle);
        Embed* get(EmbedHandle handle);
        const Embed* get(EmbedHandle handle) const;
        void clear();
        std::size_t size() const;
    };

}