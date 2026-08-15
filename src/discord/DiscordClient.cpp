#include "DiscordClient.hpp"

#include <windows.h>

#include <utility>

namespace DiscordBridge
{
    static std::wstring Utf8ToWide(const std::string& value)
    {
        if (value.empty()) return {};

        const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
        if (size <= 0) return {};

        std::wstring result(static_cast<std::size_t>(size), L'\0');

        if (MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size) <= 0) return {};

        return result;
    }

    static std::string AnsiToUtf8(const std::string& value)
    {
        if (value.empty()) return {};

        const int wideSize = MultiByteToWideChar(CP_ACP, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
        if (wideSize <= 0) return value;

        std::wstring wide(static_cast<std::size_t>(wideSize), L'\0');

        if (MultiByteToWideChar(CP_ACP, 0, value.data(), static_cast<int>(value.size()), wide.data(), wideSize) <= 0) return value;

        const int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wideSize, nullptr, 0, nullptr, nullptr);
        if (utf8Size <= 0) return value;

        std::string result(static_cast<std::size_t>(utf8Size), '\0');

        if (WideCharToMultiByte(CP_UTF8, 0, wide.data(), wideSize, result.data(), utf8Size, nullptr, nullptr) <= 0) return value;

        return result;
    }

    static std::string EscapeJson(const std::string& value)
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

    static bool FindJsonString(const std::string& json, const std::string& key, std::string& value)
    {
        const std::string search = "\"" + key + "\"";

        std::size_t position = json.find(search);
        if (position == std::string::npos) return false;

        position = json.find(':', position + search.size());
        if (position == std::string::npos) return false;

        position = json.find('"', position + 1);
        if (position == std::string::npos) return false;

        ++position;

        const std::size_t end = json.find('"', position);
        if (end == std::string::npos) return false;

        value = json.substr(position, end - position);

        return true;
    }

    DiscordClient::~DiscordClient()
    {
        shutdown();
    }

    bool DiscordClient::initialize(const std::string& token)
    {
        if (initialized_) return false;
        if (token.empty()) return false;

        token_ = token;

        const std::wstring tokenWide = Utf8ToWide(token_);
        if (tokenWide.empty())
        {
            token_.clear();
            return false;
        }

        const std::wstring headers = L"Authorization: Bot " + tokenWide + L"\r\nAccept: application/json\r\nUser-Agent: DiscordBridge/0.0.1\r\n";

        const HttpResponse response = httpClient_.get(L"discord.com", L"/api/v10/gateway/bot", headers);

        if (!response.success)
        {
            token_.clear();
            return false;
        }

        GatewayInfo gatewayInfo;

        if (!ParseGatewayInfo(response.body, gatewayInfo))
        {
            token_.clear();
            return false;
        }

        gatewayInfo_ = std::move(gatewayInfo);

        if (!gatewayClient_.connect(gatewayInfo_, token_))
        {
            gatewayInfo_ = GatewayInfo{};
            token_.clear();
            return false;
        }

        restRunning_ = true;

        try
        {
            restThread_ = std::thread(&DiscordClient::restLoop, this);
        }
        catch (...)
        {
            restRunning_ = false;
            gatewayClient_.disconnect();
            gatewayInfo_ = GatewayInfo{};
            token_.clear();
            return false;
        }

        initialized_ = true;
        connected_ = false;

        return true;
    }

    void DiscordClient::shutdown()
    {
        initialized_ = false;
        connected_ = false;

        restRunning_ = false;
        outgoingCondition_.notify_all();

        if (restThread_.joinable() && restThread_.get_id() != std::this_thread::get_id()) restThread_.join();

        {
            std::lock_guard<std::mutex> lock(outgoingMutex_);
            messageOperations_.clear();
        }

        {
            std::lock_guard<std::mutex> lock(resultMutex_);
            messageOperationResults_.clear();
        }

        gatewayClient_.disconnect();

        gatewayInfo_ = GatewayInfo{};
        token_.clear();
    }

    void DiscordClient::process()
    {
        if (!initialized_) return;
        connected_ = gatewayClient_.isReady();
    }

    bool DiscordClient::isInitialized() const
    {
        return initialized_;
    }

    bool DiscordClient::isConnected() const
    {
        return gatewayClient_.isReady();
    }

    bool DiscordClient::consumeReadyEvent()
    {
        return gatewayClient_.consumeReadyEvent();
    }

    bool DiscordClient::consumeMessageCreateEvent(std::string& userId, std::string& channelId, std::string& message)
    {
        return gatewayClient_.consumeMessageCreateEvent(userId, channelId, message);
    }

    bool DiscordClient::consumeGuildMemberAddEvent(std::string& guildId, std::string& userId)
    {
        return gatewayClient_.consumeGuildMemberAddEvent(guildId, userId);
    }

    bool DiscordClient::consumeGuildMemberRemoveEvent(std::string& guildId, std::string& userId)
    {
        return gatewayClient_.consumeGuildMemberRemoveEvent(guildId, userId);
    }

    bool DiscordClient::consumeMessageSentEvent(bool& success, std::string& channelId, std::string& messageId)
    {
        return consumeMessageOperationResult(MessageOperationType::Send, success, channelId, messageId);
    }

    bool DiscordClient::consumeMessageEditedEvent(bool& success, std::string& channelId, std::string& messageId)
    {
        return consumeMessageOperationResult(MessageOperationType::Edit, success, channelId, messageId);
    }

    bool DiscordClient::consumeMessageDeletedEvent(bool& success, std::string& channelId, std::string& messageId)
    {
        return consumeMessageOperationResult(MessageOperationType::Delete, success, channelId, messageId);
    }

    bool DiscordClient::consumeMessageOperationResult(MessageOperationType type, bool& success, std::string& channelId, std::string& messageId)
    {
        std::lock_guard<std::mutex> lock(resultMutex_);

        for (auto iterator = messageOperationResults_.begin(); iterator != messageOperationResults_.end(); ++iterator)
        {
            if (iterator->type != type) continue;

            success = iterator->success;
            channelId = std::move(iterator->channelId);
            messageId = std::move(iterator->messageId);

            messageOperationResults_.erase(iterator);

            return true;
        }

        return false;
    }

    bool DiscordClient::setStatus(int status)
    {
        return gatewayClient_.setStatus(status);
    }

    bool DiscordClient::setActivity(int type, const std::string& name, const std::string& state, const std::string& url)
    {
        return gatewayClient_.setActivity(type, name, state, url);
    }

    bool DiscordClient::clearActivity()
    {
        return gatewayClient_.clearActivity();
    }

    bool DiscordClient::setPresence(int status, int activityType, const std::string& name, const std::string& state, const std::string& url, bool afk)
    {
        return gatewayClient_.setPresence(status, activityType, name, state, url, afk);
    }

    bool DiscordClient::sendMessage(const std::string& channelId, const std::string& message)
    {
        if (!initialized_ || !restRunning_) return false;
        if (channelId.empty() || message.empty()) return false;
        if (message.size() > 2000) return false;

        {
            std::lock_guard<std::mutex> lock(outgoingMutex_);
            messageOperations_.push_back(MessageOperation{MessageOperationType::Send, channelId, "", message});
        }

        outgoingCondition_.notify_one();

        return true;
    }

    bool DiscordClient::editMessage(const std::string& channelId, const std::string& messageId, const std::string& content)
    {
        if (!initialized_ || !restRunning_) return false;
        if (channelId.empty() || messageId.empty() || content.empty()) return false;
        if (content.size() > 2000) return false;

        {
            std::lock_guard<std::mutex> lock(outgoingMutex_);
            messageOperations_.push_back(MessageOperation{MessageOperationType::Edit, channelId, messageId, content});
        }

        outgoingCondition_.notify_one();

        return true;
    }

    bool DiscordClient::deleteMessage(const std::string& channelId, const std::string& messageId)
    {
        if (!initialized_ || !restRunning_) return false;
        if (channelId.empty() || messageId.empty()) return false;

        {
            std::lock_guard<std::mutex> lock(outgoingMutex_);
            messageOperations_.push_back(MessageOperation{MessageOperationType::Delete, channelId, messageId, ""});
        }

        outgoingCondition_.notify_one();

        return true;
    }

    void DiscordClient::restLoop()
    {
        while (restRunning_)
        {
            MessageOperation operation;

            {
                std::unique_lock<std::mutex> lock(outgoingMutex_);

                outgoingCondition_.wait(lock, [this]() { return !restRunning_.load() || !messageOperations_.empty(); });

                if (!restRunning_ && messageOperations_.empty()) break;
                if (messageOperations_.empty()) continue;

                operation = std::move(messageOperations_.front());
                messageOperations_.pop_front();
            }

            bool success = false;
            std::string messageId = operation.messageId;

            switch (operation.type)
            {
                case MessageOperationType::Send:
                    success = sendMessageRequest(operation.channelId, operation.content, messageId);
                    break;

                case MessageOperationType::Edit:
                    success = editMessageRequest(operation.channelId, operation.messageId, operation.content);
                    break;

                case MessageOperationType::Delete:
                    success = deleteMessageRequest(operation.channelId, operation.messageId);
                    break;
            }

            {
                std::lock_guard<std::mutex> lock(resultMutex_);
                messageOperationResults_.push_back(MessageOperationResult{operation.type, success, operation.channelId, messageId});
            }
        }
    }

    bool DiscordClient::sendMessageRequest(const std::string& channelId, const std::string& message, std::string& messageId)
    {
        messageId.clear();

        if (token_.empty() || channelId.empty() || message.empty()) return false;

        const std::wstring tokenWide = Utf8ToWide(token_);
        const std::wstring channelWide = Utf8ToWide(channelId);

        if (tokenWide.empty() || channelWide.empty()) return false;

        const std::wstring headers = L"Authorization: Bot " + tokenWide + L"\r\nContent-Type: application/json\r\nAccept: application/json\r\nUser-Agent: DiscordBridge/0.0.1\r\n";
        const std::string body = "{\"content\":\"" + EscapeJson(AnsiToUtf8(message)) + "\"}";
        const std::wstring path = L"/api/v10/channels/" + channelWide + L"/messages";

        const HttpResponse response = httpClient_.post(L"discord.com", path, headers, body);

        if (!response.success) return false;

        FindJsonString(response.body, "id", messageId);

        return true;
    }

    bool DiscordClient::editMessageRequest(const std::string& channelId, const std::string& messageId, const std::string& content)
    {
        if (token_.empty() || channelId.empty() || messageId.empty() || content.empty()) return false;

        const std::wstring tokenWide = Utf8ToWide(token_);
        const std::wstring channelWide = Utf8ToWide(channelId);
        const std::wstring messageWide = Utf8ToWide(messageId);

        if (tokenWide.empty() || channelWide.empty() || messageWide.empty()) return false;

        const std::wstring headers = L"Authorization: Bot " + tokenWide + L"\r\nContent-Type: application/json\r\nAccept: application/json\r\nUser-Agent: DiscordBridge/0.0.1\r\n";
        const std::string body = "{\"content\":\"" + EscapeJson(AnsiToUtf8(content)) + "\"}";
        const std::wstring path = L"/api/v10/channels/" + channelWide + L"/messages/" + messageWide;

        return httpClient_.patch(L"discord.com", path, headers, body).success;
    }

    bool DiscordClient::deleteMessageRequest(const std::string& channelId, const std::string& messageId)
    {
        if (token_.empty() || channelId.empty() || messageId.empty()) return false;

        const std::wstring tokenWide = Utf8ToWide(token_);
        const std::wstring channelWide = Utf8ToWide(channelId);
        const std::wstring messageWide = Utf8ToWide(messageId);

        if (tokenWide.empty() || channelWide.empty() || messageWide.empty()) return false;

        const std::wstring headers = L"Authorization: Bot " + tokenWide + L"\r\nAccept: application/json\r\nUser-Agent: DiscordBridge/0.0.1\r\n";
        const std::wstring path = L"/api/v10/channels/" + channelWide + L"/messages/" + messageWide;

        return httpClient_.del(L"discord.com", path, headers).success;
    }

    const std::string& DiscordClient::getToken() const
    {
        return token_;
    }

    const GatewayInfo& DiscordClient::getGatewayInfo() const
    {
        return gatewayInfo_;
    }
}