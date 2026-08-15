#include "DiscordClient.hpp"

#include <windows.h>

#include <utility>

namespace DiscordBridge
{
    namespace
    {
        constexpr wchar_t DISCORD_HOST[] = L"discord.com";
        constexpr wchar_t API_BASE[] = L"/api/v10";
        constexpr wchar_t USER_AGENT[] = L"DiscordBridge/0.0.1";

        std::wstring Utf8ToWide(const std::string& value)
        {
            if (value.empty()) return {};

            const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
            if (size <= 0) return {};

            std::wstring result(static_cast<std::size_t>(size), L'\0');
            if (MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size) <= 0) return {};

            return result;
        }

        std::string AnsiToUtf8(const std::string& value)
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

        bool FindJsonString(const std::string& json, const std::string& key, std::string& value)
        {
            const std::string search = "\"" + key + "\"";

            std::size_t position = json.find(search);
            if (position == std::string::npos) return false;

            position = json.find(':', position + search.size());
            if (position == std::string::npos) return false;

            position = json.find('"', position + 1);
            if (position == std::string::npos) return false;

            const std::size_t end = json.find('"', ++position);
            if (end == std::string::npos) return false;

            value.assign(json, position, end - position);
            return true;
        }

        std::wstring CreateHeaders(const std::string& token, bool contentType = false)
        {
            const std::wstring tokenWide = Utf8ToWide(token);
            if (tokenWide.empty()) return {};

            std::wstring headers = L"Authorization: Bot " + tokenWide + L"\r\n";

            if (contentType) headers += L"Content-Type: application/json\r\n";

            headers += L"Accept: application/json\r\nUser-Agent: ";
            headers += USER_AGENT;
            headers += L"\r\n";

            return headers;
        }

        std::wstring CreateMessagePath(const std::string& channelId, const std::string& messageId = {})
        {
            const std::wstring channelWide = Utf8ToWide(channelId);
            if (channelWide.empty()) return {};

            std::wstring path = API_BASE;
            path += L"/channels/";
            path += channelWide;
            path += L"/messages";

            if (!messageId.empty())
            {
                const std::wstring messageWide = Utf8ToWide(messageId);
                if (messageWide.empty()) return {};

                path += L"/";
                path += messageWide;
            }

            return path;
        }
    }

    DiscordClient::~DiscordClient()
    {
        shutdown();
    }

    bool DiscordClient::initialize(const std::string& token)
    {
        if (initialized_ || token.empty()) return false;

        token_ = token;

        const std::wstring headers = CreateHeaders(token_);
        if (headers.empty())
        {
            token_.clear();
            return false;
        }

        const HttpResponse response = httpClient_.get(DISCORD_HOST, std::wstring(API_BASE) + L"/gateway/bot", headers);

        GatewayInfo gatewayInfo;
        if (!response.success || !ParseGatewayInfo(response.body, gatewayInfo))
        {
            token_.clear();
            return false;
        }

        gatewayInfo_ = std::move(gatewayInfo);

        if (!gateway_.connect(gatewayInfo_, token_))
        {
            gatewayInfo_ = {};
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
            gateway_.disconnect();
            gatewayInfo_ = {};
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

        gateway_.disconnect();
        gatewayInfo_ = {};
        token_.clear();
    }

    void DiscordClient::process()
    {
        if (initialized_) connected_ = gateway_.isReady();
    }

    bool DiscordClient::isInitialized() const
    {
        return initialized_;
    }

    bool DiscordClient::isConnected() const
    {
        return gateway_.isReady();
    }

    bool DiscordClient::consumeReadyEvent()
    {
        return gateway_.consumeReadyEvent();
    }

    bool DiscordClient::consumeMessageCreateEvent(std::string& userId, std::string& channelId, std::string& message)
    {
        return gateway_.consumeMessageCreateEvent(userId, channelId, message);
    }

    bool DiscordClient::consumeGuildMemberAddEvent(std::string& guildId, std::string& userId)
    {
        return gateway_.consumeGuildMemberAddEvent(guildId, userId);
    }

    bool DiscordClient::consumeGuildMemberRemoveEvent(std::string& guildId, std::string& userId)
    {
        return gateway_.consumeGuildMemberRemoveEvent(guildId, userId);
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

    bool DiscordClient::consumeEmbedSentEvent(bool& success, std::string& channelId, std::string& messageId)
    {
        return consumeMessageOperationResult(MessageOperationType::SendEmbed, success, channelId, messageId);
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
        return gateway_.setStatus(status);
    }

    bool DiscordClient::setActivity(int type, const std::string& name, const std::string& state, const std::string& url)
    {
        return gateway_.setActivity(type, name, state, url);
    }

    bool DiscordClient::clearActivity()
    {
        return gateway_.clearActivity();
    }

    bool DiscordClient::setPresence(int status, int activityType, const std::string& name, const std::string& state, const std::string& url, bool afk)
    {
        return gateway_.setPresence(status, activityType, name, state, url, afk);
    }

    bool DiscordClient::sendMessage(const std::string& channelId, const std::string& message)
    {
        if (!initialized_ || !restRunning_ || channelId.empty() || message.empty() || message.size() > 2000) return false;

        {
            std::lock_guard<std::mutex> lock(outgoingMutex_);
            messageOperations_.push_back({MessageOperationType::Send, channelId, {}, message});
        }

        outgoingCondition_.notify_one();
        return true;
    }

    bool DiscordClient::editMessage(const std::string& channelId, const std::string& messageId, const std::string& content)
    {
        if (!initialized_ || !restRunning_ || channelId.empty() || messageId.empty() || content.empty() || content.size() > 2000) return false;

        {
            std::lock_guard<std::mutex> lock(outgoingMutex_);
            messageOperations_.push_back({MessageOperationType::Edit, channelId, messageId, content});
        }

        outgoingCondition_.notify_one();
        return true;
    }

    bool DiscordClient::deleteMessage(const std::string& channelId, const std::string& messageId)
    {
        if (!initialized_ || !restRunning_ || channelId.empty() || messageId.empty()) return false;

        {
            std::lock_guard<std::mutex> lock(outgoingMutex_);
            messageOperations_.push_back({MessageOperationType::Delete, channelId, messageId, {}});
        }

        outgoingCondition_.notify_one();
        return true;
    }

    bool DiscordClient::sendEmbed(const std::string& channelId, const std::string& embedJson)
    {
        if (!initialized_ || !restRunning_ || channelId.empty() || embedJson.empty()) return false;

        {
            std::lock_guard<std::mutex> lock(outgoingMutex_);
            messageOperations_.push_back({MessageOperationType::SendEmbed, channelId, {}, embedJson});
        }

        outgoingCondition_.notify_one();
        return true;
    }

    void DiscordClient::restLoop()
    {
        while (true)
        {
            MessageOperation operation;

            {
                std::unique_lock<std::mutex> lock(outgoingMutex_);
                outgoingCondition_.wait(lock, [this]() { return !restRunning_.load() || !messageOperations_.empty(); });

                if (!restRunning_ && messageOperations_.empty()) break;

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

                case MessageOperationType::SendEmbed:
                    success = sendEmbedRequest(operation.channelId, operation.content, messageId);
                    break;
            }

            std::lock_guard<std::mutex> lock(resultMutex_);
            messageOperationResults_.push_back({operation.type, success, std::move(operation.channelId), std::move(messageId)});
        }
    }

    bool DiscordClient::sendMessageRequest(const std::string& channelId, const std::string& message, std::string& messageId)
    {
        messageId.clear();

        if (token_.empty() || channelId.empty() || message.empty()) return false;

        const std::wstring path = CreateMessagePath(channelId);
        const std::wstring headers = CreateHeaders(token_, true);

        if (path.empty() || headers.empty()) return false;

        const std::string body = "{\"content\":\"" + EscapeJson(AnsiToUtf8(message)) + "\"}";
        const HttpResponse response = httpClient_.post(DISCORD_HOST, path, headers, body);

        if (!response.success) return false;

        FindJsonString(response.body, "id", messageId);
        return true;
    }

    bool DiscordClient::editMessageRequest(const std::string& channelId, const std::string& messageId, const std::string& content)
    {
        if (token_.empty() || channelId.empty() || messageId.empty() || content.empty()) return false;

        const std::wstring path = CreateMessagePath(channelId, messageId);
        const std::wstring headers = CreateHeaders(token_, true);

        if (path.empty() || headers.empty()) return false;

        const std::string body = "{\"content\":\"" + EscapeJson(AnsiToUtf8(content)) + "\"}";
        return httpClient_.patch(DISCORD_HOST, path, headers, body).success;
    }

    bool DiscordClient::deleteMessageRequest(const std::string& channelId, const std::string& messageId)
    {
        if (token_.empty() || channelId.empty() || messageId.empty()) return false;

        const std::wstring path = CreateMessagePath(channelId, messageId);
        const std::wstring headers = CreateHeaders(token_);

        if (path.empty() || headers.empty()) return false;

        return httpClient_.del(DISCORD_HOST, path, headers).success;
    }

    bool DiscordClient::sendEmbedRequest(const std::string& channelId, const std::string& embedJson, std::string& messageId)
    {
        messageId.clear();

        if (token_.empty() || channelId.empty() || embedJson.empty()) return false;

        const std::wstring path = CreateMessagePath(channelId);
        const std::wstring headers = CreateHeaders(token_, true);

        if (path.empty() || headers.empty()) return false;

        const std::string body = "{\"embeds\":[" + embedJson + "]}";
        const HttpResponse response = httpClient_.post(DISCORD_HOST, path, headers, body);

        if (!response.success) return false;

        FindJsonString(response.body, "id", messageId);
        return true;
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