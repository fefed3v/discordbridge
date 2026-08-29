#include "Modal.hpp"

namespace DiscordBridge
{
    std::string Modal::escapeJson(const std::string &value)
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

    bool Modal::addInput(const TextInput &input)
    {
        if (components_.size() >= 5 || input.customId.empty() || input.customId.size() > 100 || input.label.empty() || input.label.size() > 45 || input.description.size() > 100 || input.style < 1 || input.style > 2 || input.minLength < 0 || input.maxLength < 1 || input.maxLength > 4000 || input.minLength > input.maxLength)
            return false;
        std::string child = "{\"type\":4,\"custom_id\":\"" + escapeJson(input.customId) + "\",\"style\":" + std::to_string(input.style) + ",\"required\":" + (input.required ? "true" : "false") + ",\"min_length\":" + std::to_string(input.minLength) + ",\"max_length\":" + std::to_string(input.maxLength);
        if (!input.placeholder.empty())
            child += ",\"placeholder\":\"" + escapeJson(input.placeholder) + "\"";
        if (!input.value.empty())
            child += ",\"value\":\"" + escapeJson(input.value) + "\"";
        child += '}';
        return addLabeledComponent(input.label, input.description, child);
    }

    bool Modal::addFileUpload(const std::string &customId, const std::string &label, const std::string &description, int minValues, int maxValues, bool required)
    {
        if (customId.empty() || customId.size() > 100 || minValues < 0 || maxValues < 1 || maxValues > 10 || minValues > maxValues)
            return false;
        const std::string child = "{\"type\":19,\"custom_id\":\"" + escapeJson(customId) + "\",\"min_values\":" + std::to_string(minValues) + ",\"max_values\":" + std::to_string(maxValues) + ",\"required\":" + (required ? "true" : "false") + "}";
        return addLabeledComponent(label, description, child);
    }

    bool Modal::addTextDisplay(const std::string &content)
    {
        if (content.empty() || content.size() > 4000 || components_.size() >= 5)
            return false;
        components_.push_back("{\"type\":10,\"content\":\"" + escapeJson(content) + "\"}");
        return true;
    }

    bool Modal::addLabeledComponent(const std::string &label, const std::string &description, const std::string &componentJson)
    {
        if (label.empty() || label.size() > 45 || description.size() > 100 || componentJson.empty() || components_.size() >= 5)
            return false;
        std::string json = "{\"type\":18,\"label\":\"" + escapeJson(label) + "\"";
        if (!description.empty())
            json += ",\"description\":\"" + escapeJson(description) + "\"";
        json += ",\"component\":" + componentJson + '}';
        components_.push_back(std::move(json));
        return true;
    }

    bool Modal::isValid() const { return !customId_.empty() && customId_.size() <= 100 && !title_.empty() && title_.size() <= 45 && !components_.empty() && components_.size() <= 5; }
    std::string Modal::toJson() const
    {
        if (!isValid())
            return {};
        std::string json = "{\"custom_id\":\"" + escapeJson(customId_) + "\",\"title\":\"" + escapeJson(title_) + "\",\"components\":[";
        for (std::size_t i = 0; i < components_.size(); ++i)
        {
            if (i)
                json += ',';
            json += components_[i];
        }
        return json + "]}";
    }
    ModalHandle ModalManager::create()
    {
        while (next_ == 0 || items_.count(next_))
            ++next_;
        const auto handle = next_++;
        items_[handle] = std::make_unique<Modal>();
        return handle;
    }
    bool ModalManager::destroy(ModalHandle handle) { return handle && items_.erase(handle) > 0; }
    Modal *ModalManager::get(ModalHandle handle)
    {
        const auto it = items_.find(handle);
        return it == items_.end() ? nullptr : it->second.get();
    }
    const Modal *ModalManager::get(ModalHandle handle) const
    {
        const auto it = items_.find(handle);
        return it == items_.end() ? nullptr : it->second.get();
    }
}
