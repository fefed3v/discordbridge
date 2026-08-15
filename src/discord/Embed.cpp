#include "Embed.hpp"

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

        void AppendProperty(std::string& json, bool& comma, const std::string& property)
        {
            if (comma) json += ',';
            json += property;
            comma = true;
        }
    }

    bool EmbedAuthor::empty() const
    {
        return name.empty();
    }

    void EmbedAuthor::clear()
    {
        name.clear();
        url.clear();
        iconUrl.clear();
    }

    bool EmbedFooter::empty() const
    {
        return text.empty();
    }

    void EmbedFooter::clear()
    {
        text.clear();
        iconUrl.clear();
    }

    void Embed::setTitle(const std::string& title)
    {
        title_ = title;
    }

    void Embed::setDescription(const std::string& description)
    {
        description_ = description;
    }

    void Embed::setUrl(const std::string& url)
    {
        url_ = url;
    }

    void Embed::setColor(std::uint32_t color)
    {
        color_ = color & 0xFFFFFF;
    }

    void Embed::setAuthor(const std::string& name, const std::string& url, const std::string& iconUrl)
    {
        author_ = {name, url, iconUrl};
    }

    void Embed::clearAuthor()
    {
        author_.clear();
    }

    void Embed::setThumbnail(const std::string& url)
    {
        thumbnailUrl_ = url;
    }

    void Embed::clearThumbnail()
    {
        thumbnailUrl_.clear();
    }

    void Embed::setImage(const std::string& url)
    {
        imageUrl_ = url;
    }

    void Embed::clearImage()
    {
        imageUrl_.clear();
    }

    void Embed::setFooter(const std::string& text, const std::string& iconUrl)
    {
        footer_ = {text, iconUrl};
    }

    void Embed::clearFooter()
    {
        footer_.clear();
    }

    bool Embed::addField(const std::string& name, const std::string& value, bool inlineField)
    {
        if (name.empty() || value.empty() || fields_.size() >= 25) return false;

        fields_.push_back({name, value, inlineField});
        return true;
    }

    void Embed::clearFields()
    {
        fields_.clear();
    }

    void Embed::clear()
    {
        title_.clear();
        description_.clear();
        url_.clear();
        thumbnailUrl_.clear();
        imageUrl_.clear();
        color_ = 0;
        author_.clear();
        footer_.clear();
        fields_.clear();
    }

    bool Embed::empty() const
    {
        return title_.empty() && description_.empty() && url_.empty() && thumbnailUrl_.empty() && imageUrl_.empty() && author_.empty() && footer_.empty() && fields_.empty();
    }

    const std::string& Embed::getTitle() const
    {
        return title_;
    }

    const std::string& Embed::getDescription() const
    {
        return description_;
    }

    const std::string& Embed::getUrl() const
    {
        return url_;
    }

    std::uint32_t Embed::getColor() const
    {
        return color_;
    }

    const EmbedAuthor& Embed::getAuthor() const
    {
        return author_;
    }

    const std::string& Embed::getThumbnail() const
    {
        return thumbnailUrl_;
    }

    const std::string& Embed::getImage() const
    {
        return imageUrl_;
    }

    const EmbedFooter& Embed::getFooter() const
    {
        return footer_;
    }

    const std::vector<EmbedField>& Embed::getFields() const
    {
        return fields_;
    }

    std::string Embed::toJson() const
    {
        std::string json = "{";
        bool comma = false;

        if (!title_.empty()) AppendProperty(json, comma, "\"title\":\"" + EscapeJson(title_) + "\"");
        if (!description_.empty()) AppendProperty(json, comma, "\"description\":\"" + EscapeJson(description_) + "\"");
        if (!url_.empty()) AppendProperty(json, comma, "\"url\":\"" + EscapeJson(url_) + "\"");
        if (color_ != 0) AppendProperty(json, comma, "\"color\":" + std::to_string(color_));

        if (!author_.empty())
        {
            std::string author = "\"author\":{\"name\":\"" + EscapeJson(author_.name) + "\"";
            if (!author_.url.empty()) author += ",\"url\":\"" + EscapeJson(author_.url) + "\"";
            if (!author_.iconUrl.empty()) author += ",\"icon_url\":\"" + EscapeJson(author_.iconUrl) + "\"";
            author += "}";

            AppendProperty(json, comma, author);
        }

        if (!thumbnailUrl_.empty()) AppendProperty(json, comma, "\"thumbnail\":{\"url\":\"" + EscapeJson(thumbnailUrl_) + "\"}");
        if (!imageUrl_.empty()) AppendProperty(json, comma, "\"image\":{\"url\":\"" + EscapeJson(imageUrl_) + "\"}");

        if (!footer_.empty())
        {
            std::string footer = "\"footer\":{\"text\":\"" + EscapeJson(footer_.text) + "\"";
            if (!footer_.iconUrl.empty()) footer += ",\"icon_url\":\"" + EscapeJson(footer_.iconUrl) + "\"";
            footer += "}";

            AppendProperty(json, comma, footer);
        }

        if (!fields_.empty())
        {
            std::string fields = "\"fields\":[";

            for (std::size_t index = 0; index < fields_.size(); ++index)
            {
                if (index > 0) fields += ',';

                const EmbedField& field = fields_[index];

                fields += "{\"name\":\"" + EscapeJson(field.name) + "\",\"value\":\"" + EscapeJson(field.value) + "\",\"inline\":";
                fields += field.inlineField ? "true" : "false";
                fields += "}";
            }

            fields += "]";
            AppendProperty(json, comma, fields);
        }

        json += "}";
        return json;
    }

    EmbedHandle EmbedManager::create()
    {
        if (nextHandle_ == 0) nextHandle_ = 1;

        const EmbedHandle start = nextHandle_;
        EmbedHandle handle = start;

        do
        {
            if (embeds_.find(handle) == embeds_.end())
            {
                embeds_.emplace(handle, std::make_unique<Embed>());

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

    bool EmbedManager::destroy(EmbedHandle handle)
    {
        return handle != 0 && embeds_.erase(handle) != 0;
    }

    Embed* EmbedManager::get(EmbedHandle handle)
    {
        const auto iterator = embeds_.find(handle);
        return iterator != embeds_.end() ? iterator->second.get() : nullptr;
    }

    const Embed* EmbedManager::get(EmbedHandle handle) const
    {
        const auto iterator = embeds_.find(handle);
        return iterator != embeds_.end() ? iterator->second.get() : nullptr;
    }

    void EmbedManager::clear()
    {
        embeds_.clear();
        nextHandle_ = 1;
    }

    std::size_t EmbedManager::size() const
    {
        return embeds_.size();
    }
}