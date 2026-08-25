#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
namespace DiscordBridge {
using SelectMenuHandle=std::uint32_t;
struct SelectOption { std::string label,value,description,emoji; bool isDefault{false}; };
class SelectMenu { public:
 void setCustomId(const std::string& v){customId_=v;} void setPlaceholder(const std::string& v){placeholder_=v;} void setMinValues(int v){minValues_=v;} void setMaxValues(int v){maxValues_=v;} void setDisabled(bool v){disabled_=v;}
 bool addOption(const SelectOption& o); void clearOptions(){options_.clear();} bool isValid() const; std::string toJson() const;
 private: std::string customId_,placeholder_; int minValues_{1},maxValues_{1}; bool disabled_{false}; std::vector<SelectOption> options_;
};
class SelectMenuManager { public: SelectMenuHandle create(); bool destroy(SelectMenuHandle); SelectMenu* get(SelectMenuHandle); const SelectMenu* get(SelectMenuHandle) const; void clear(){items_.clear();next_=1;} private: std::unordered_map<SelectMenuHandle,std::unique_ptr<SelectMenu>> items_; SelectMenuHandle next_{1}; };
}
