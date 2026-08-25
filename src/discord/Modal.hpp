#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
namespace DiscordBridge { using ModalHandle=std::uint32_t; struct TextInput{std::string customId,label,placeholder,value;int style{1};int minLength{0},maxLength{4000};bool required{true};}; class Modal{public:void setCustomId(const std::string&v){customId_=v;}void setTitle(const std::string&v){title_=v;}bool addInput(const TextInput&);void clearInputs(){inputs_.clear();}bool isValid()const;std::string toJson()const;private:std::string customId_,title_;std::vector<TextInput>inputs_;}; class ModalManager{public:ModalHandle create();bool destroy(ModalHandle);Modal*get(ModalHandle);const Modal*get(ModalHandle)const;void clear(){items_.clear();next_=1;}private:std::unordered_map<ModalHandle,std::unique_ptr<Modal>>items_;ModalHandle next_{1};}; }
