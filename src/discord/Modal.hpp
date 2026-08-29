#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace DiscordBridge
{
    using ModalHandle = std::uint32_t;

    struct TextInput
    {
        std::string customId, label, description, placeholder, value;
        int style{1};
        int minLength{0}, maxLength{4000};
        bool required{true};
    };

    class Modal
    {
    public:
        void setCustomId(const std::string &value) { customId_ = value; }
        void setTitle(const std::string &value) { title_ = value; }
        bool addInput(const TextInput &input);
        bool addFileUpload(const std::string &customId, const std::string &label, const std::string &description, int minValues, int maxValues, bool required);
        bool addTextDisplay(const std::string &content);
        bool addLabeledComponent(const std::string &label, const std::string &description, const std::string &componentJson);
        bool addRawComponent(const std::string &componentJson) { if (componentJson.empty() || components_.size() >= 5) return false; components_.push_back(componentJson); return true; }
        void clearInputs() { components_.clear(); }
        bool isValid() const;
        std::string toJson() const;

    private:
        static std::string escapeJson(const std::string &value);
        std::string customId_, title_;
        std::vector<std::string> components_;
    };

    class ModalManager
    {
    public:
        ModalHandle create();
        bool destroy(ModalHandle handle);
        Modal *get(ModalHandle handle);
        const Modal *get(ModalHandle handle) const;
        void clear()
        {
            items_.clear();
            next_ = 1;
        }

    private:
        std::unordered_map<ModalHandle, std::unique_ptr<Modal>> items_;
        ModalHandle next_{1};
    };
}
