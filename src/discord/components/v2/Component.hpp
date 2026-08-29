#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
namespace DiscordBridge
{
using ComponentHandle = std::uint32_t;
struct ComponentOption { std::string label, value, description, emoji; bool isDefault{false}; };
struct MediaItem { std::string url, description; bool spoiler{false}; };
class Component
{
public:
    explicit Component(int type) : type_(type) {}
    int type() const { return type_; }
    bool setCustomId(const std::string &v); bool setLabel(const std::string &v); bool setDescription(const std::string &v); bool setContent(const std::string &v); bool setPlaceholder(const std::string &v); bool setValue(const std::string &v); bool setUrl(const std::string &v); bool setEmoji(const std::string &v); bool setSkuId(const std::string &v);
    bool setStyle(int v); bool setRange(int minValues, int maxValues); bool setLength(int minLength, int maxLength); bool setAccentColor(int v); bool setSpacing(int v);
    void setDisabled(bool v) { disabled_=v; } void setRequired(bool v) { required_=v; } void setSpoiler(bool v) { spoiler_=v; } void setDivider(bool v) { divider_=v; }
    bool addOption(const std::string &label, const std::string &value, const std::string &description, const std::string &emoji = {}, bool isDefault = false); bool addMedia(const std::string &url, const std::string &description, bool spoiler); bool addChannelType(int type); bool addChild(ComponentHandle child, const class ComponentManager &manager); bool setAccessory(ComponentHandle child, const class ComponentManager &manager);
    std::string toJson(const class ComponentManager &manager) const;
private:
    int type_{0}, style_{1}, minValues_{1}, maxValues_{1}, minLength_{0}, maxLength_{4000}, accentColor_{-1}, spacing_{1};
    bool disabled_{false}, required_{true}, spoiler_{false}, divider_{true};
    std::string customId_, label_, description_, content_, placeholder_, value_, url_, emoji_, skuId_;
    std::vector<ComponentOption> options_; std::vector<MediaItem> media_; std::vector<int> channelTypes_; std::vector<ComponentHandle> children_; ComponentHandle accessory_{0};
};
class ComponentManager
{
public:
    ComponentHandle create(int type); bool destroy(ComponentHandle handle); Component *get(ComponentHandle handle); const Component *get(ComponentHandle handle) const; void clear(); std::size_t size() const { return components_.size(); }
private:
    std::unordered_map<ComponentHandle,std::unique_ptr<Component>> components_; ComponentHandle next_{1};
};
}
