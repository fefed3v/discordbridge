#include "DiscordClient.hpp"

#include <windows.h>

#include <cctype>
#include <utility>

namespace DiscordBridge
{
    namespace
    {
        std::wstring Utf8ToWide(const std::string& value)
        {
            if (value.empty()) return {};

            const int size = MultiByteToWideChar(
                CP_UTF8,
                0,
                value.data(),
                static_cast<int>(value.size()),
                nullptr,
                0
            );

            if (size <= 0) return {};

            std::wstring result(static_cast<std::size_t>(size), L'\0');

            if (MultiByteToWideChar(
                CP_UTF8,
                0,
                value.data(),
                static_cast<int>(value.size()),
                result.data(),
                size
            ) <= 0)
            {
                return {};
            }

            return result;
        }

        std::string AnsiToUtf8(const std::string& value)
        {
            if (value.empty()) return {};

            const int wideSize = MultiByteToWideChar(
                CP_ACP,
                0,
                value.data(),
                static_cast<int>(value.size()),
                nullptr,
                0
            );

            if (wideSize <= 0) return value;

            std::wstring wide(static_cast<std::size_t>(wideSize), L'\0');

            if (MultiByteToWideChar(
                CP_ACP,
                0,
                value.data(),
                static_cast<int>(value.size()),
                wide.data(),
                wideSize
            ) <= 0)
            {
                return value;
            }

            const int utf8Size = WideCharToMultiByte(
                CP_UTF8,
                0,
                wide.data(),
                wideSize,
                nullptr,
                0,
                nullptr,
                nullptr
            );

            if (utf8Size <= 0) return value;

            std::string result(static_cast<std::size_t>(utf8Size), '\0');

            if (WideCharToMultiByte(
                CP_UTF8,
                0,
                wide.data(),
                wideSize,
                result.data(),
                utf8Size,
                nullptr,
                nullptr
            ) <= 0)
            {
                return value;
            }

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

        bool FindRootString(const std::string& json, const std::string& key, std::string& value)
        {
            std::size_t position = 0;

            while (position < json.size() && std::isspace(static_cast<unsigned char>(json[position]))) ++position;

            if (position >= json.size() || json[position] != '{') return false;

            ++position;

            int depth = 1;
            bool insideString = false;
            bool escaped = false;

            while (position < json.size())
            {
                if (depth != 1)
                {
                    const char character = json[position++];

                    if (insideString)
                    {
                        if (escaped) escaped = false;
                        else if (character == '\\') escaped = true;
                        else if (character == '"') insideString = false;
                    }
                    else
                    {
                        if (character == '"') insideString = true;
                        else if (character == '{' || character == '[') ++depth;
                        else if (character == '}' || character == ']') --depth;
                    }

                    continue;
                }

                while (position < json.size())
                {
                    const char character = json[position];

                    if (std::isspace(static_cast<unsigned char>(character)) || character == ',')
                    {
                        ++position;
                        continue;
                    }

                    break;
                }

                if (position >= json.size() || json[position] == '}') break;

                if (json[position] != '"') return false;

                ++position;

                std::string currentKey;

                while (position < json.size())
                {
                    const char character = json[position++];

                    if (character == '\\')
                    {
                        if (position < json.size())
                        {
                            currentKey.push_back(json[position++]);
                        }

                        continue;
                    }

                    if (character == '"') break;

                    currentKey.push_back(character);
                }

                while (position < json.size() && std::isspace(static_cast<unsigned char>(json[position]))) ++position;

                if (position >= json.size() || json[position] != ':') return false;

                ++position;

                while (position < json.size() && std::isspace(static_cast<unsigned char>(json[position]))) ++position;

                if (position >= json.size()) return false;

                if (currentKey == key)
                {
                    if (json[position] != '"') return false;

                    ++position;

                    std::string result;
                    bool valueEscaped = false;

                    while (position < json.size())
                    {
                        const char character = json[position++];

                        if (valueEscaped)
                        {
                            switch (character)
                            {
                                case '"': result.push_back('"'); break;
                                case '\\': result.push_back('\\'); break;
                                case '/': result.push_back('/'); break;
                                case 'b': result.push_back('\b'); break;
                                case 'f': result.push_back('\f'); break;
                                case 'n': result.push_back('\n'); break;
                                case 'r': result.push_back('\r'); break;
                                case 't': result.push_back('\t'); break;
                                default: result.push_back(character); break;
                            }

                            valueEscaped = false;
                            continue;
                        }

                        if (character == '\\')
                        {
                            valueEscaped = true;
                            continue;
                        }

                        if (character == '"')
                        {
                            value = std::move(result);
                            return true;
                        }

                        result.push_back(character);
                    }

                    return false;
                }

                if (json[position] == '"')
                {
                    ++position;

                    bool stringEscaped = false;

                    while (position < json.size())
                    {
                        const char character = json[position++];

                        if (stringEscaped)
                        {
                            stringEscaped = false;
                            continue;
                        }

                        if (character == '\\')
                        {
                            stringEscaped = true;
                            continue;
                        }

                        if (character == '"') break;
                    }
                }
                else if (json[position] == '{' || json[position] == '[')
                {
                    const char opening = json[position];
                    const char closing = opening == '{' ? '}' : ']';

                    int nestedDepth = 1;
                    ++position;

                    bool nestedString = false;
                    bool nestedEscaped = false;

                    while (position < json.size() && nestedDepth > 0)
                    {
                        const char character = json[position++];

                        if (nestedString)
                        {
                            if (nestedEscaped) nestedEscaped = false;
                            else if (character == '\\') nestedEscaped = true;
                            else if (character == '"') nestedString = false;

                            continue;
                        }

                        if (character == '"')
                        {
                            nestedString = true;
                            continue;
                        }

                        if (character == opening) ++nestedDepth;
                        else if (character == closing) --nestedDepth;
                    }
                }
                else
                {
                    while (position < json.size() && json[position] != ',' && json[position] != '}') ++position;
                }
            }

            return false;
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

        const std::wstring tokenWide = Utf8ToWide(token_);

        if (tokenWide.empty())
        {
            token_.clear();
            return false;
        }

        const std::wstring headers =
            L"Authorization: Bot " + tokenWide +
            L"\r\nAccept: application/json"
            L"\r\nUser-Agent: DiscordBridge/0.0.1"
            L"\r\n";

        const HttpResponse response = httpClient_.get(
            L"discord.com",
            L"/api/v10/gateway/bot",
            headers
        );

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

        if (!gateway_.connect(gatewayInfo_, token_))
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

            gateway_.disconnect();

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

        gateway_.disconnect();

        gatewayInfo_ = GatewayInfo{};
        token_.clear();
    }

    void DiscordClient::process()
    {
        if (!initialized_) return;

        connected_ = gateway_.isReady();
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

    bool DiscordClient::consumeButtonClickEvent(std::string& interactionId, std::string& interactionToken, std::string& userId, std::string& channelId, std::string& customId)
    {
        return gateway_.consumeButtonClickEvent(
            interactionId,
            interactionToken,
            userId,
            channelId,
            customId
        );
    }

    bool DiscordClient::consumeMessageSentEvent(bool& success, std::string& channelId, std::string& messageId)
    {
        return consumeMessageOperationResult(
            MessageOperationType::Send,
            success,
            channelId,
            messageId
        );
    }

    bool DiscordClient::consumeMessageEditedEvent(bool& success, std::string& channelId, std::string& messageId)
    {
        return consumeMessageOperationResult(
            MessageOperationType::Edit,
            success,
            channelId,
            messageId
        );
    }

    bool DiscordClient::consumeMessageDeletedEvent(bool& success, std::string& channelId, std::string& messageId)
    {
        return consumeMessageOperationResult(
            MessageOperationType::Delete,
            success,
            channelId,
            messageId
        );
    }

    bool DiscordClient::consumeEmbedSentEvent(bool& success, std::string& channelId, std::string& messageId)
    {
        return consumeMessageOperationResult(
            MessageOperationType::SendEmbed,
            success,
            channelId,
            messageId
        );
    }

    bool DiscordClient::consumeComponentsSentEvent(bool& success, std::string& channelId, std::string& messageId)
    {
        return consumeMessageOperationResult(
            MessageOperationType::SendComponents,
            success,
            channelId,
            messageId
        );
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
        return gateway_.setPresence(
            status,
            activityType,
            name,
            state,
            url,
            afk
        );
    }

    bool DiscordClient::sendMessage(const std::string& channelId, const std::string& message)
    {
        if (!initialized_ || !restRunning_) return false;
        if (channelId.empty() || message.empty()) return false;
        if (message.size() > 2000) return false;

        {
            std::lock_guard<std::mutex> lock(outgoingMutex_);

            messageOperations_.push_back(
                MessageOperation{
                    MessageOperationType::Send,
                    channelId,
                    "",
                    message
                }
            );
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

            messageOperations_.push_back(
                MessageOperation{
                    MessageOperationType::Edit,
                    channelId,
                    messageId,
                    content
                }
            );
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

            messageOperations_.push_back(
                MessageOperation{
                    MessageOperationType::Delete,
                    channelId,
                    messageId,
                    ""
                }
            );
        }

        outgoingCondition_.notify_one();

        return true;
    }

    bool DiscordClient::sendEmbed(const std::string& channelId, const std::string& embedJson)
    {
        if (!initialized_ || !restRunning_) return false;
        if (channelId.empty() || embedJson.empty()) return false;

        {
            std::lock_guard<std::mutex> lock(outgoingMutex_);

            messageOperations_.push_back(
                MessageOperation{
                    MessageOperationType::SendEmbed,
                    channelId,
                    "",
                    embedJson
                }
            );
        }

        outgoingCondition_.notify_one();

        return true;
    }

    bool DiscordClient::sendComponents(const std::string& channelId, const std::string& componentsJson)
    {
        if (!initialized_ || !restRunning_) return false;
        if (channelId.empty() || componentsJson.empty()) return false;

        {
            std::lock_guard<std::mutex> lock(outgoingMutex_);

            messageOperations_.push_back(
                MessageOperation{
                    MessageOperationType::SendComponents,
                    channelId,
                    "",
                    componentsJson
                }
            );
        }

        outgoingCondition_.notify_one();

        return true;
    }

    bool DiscordClient::acknowledgeInteraction(const std::string& interactionId, const std::string& interactionToken)
    {
        if (!initialized_) return false;
        if (interactionId.empty() || interactionToken.empty()) return false;

        const std::wstring interactionWide = Utf8ToWide(interactionId);
        const std::wstring tokenWide = Utf8ToWide(interactionToken);

        if (interactionWide.empty() || tokenWide.empty()) return false;

        const std::wstring headers =
            L"Content-Type: application/json"
            L"\r\nAccept: application/json"
            L"\r\nUser-Agent: DiscordBridge/0.0.1"
            L"\r\n";

        const std::wstring path =
            L"/api/v10/interactions/" +
            interactionWide +
            L"/" +
            tokenWide +
            L"/callback";

        const std::string body = "{\"type\":6}";

        const HttpResponse response = httpClient_.post(
            L"discord.com",
            path,
            headers,
            body
        );

        return response.success;
    }

    void DiscordClient::restLoop()
    {
        while (restRunning_)
        {
            MessageOperation operation;

            {
                std::unique_lock<std::mutex> lock(outgoingMutex_);

                outgoingCondition_.wait(lock, [this]()
                {
                    return !restRunning_.load() || !messageOperations_.empty();
                });

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
                    success = sendMessageRequest(
                        operation.channelId,
                        operation.content,
                        messageId
                    );
                    break;

                case MessageOperationType::Edit:
                    success = editMessageRequest(
                        operation.channelId,
                        operation.messageId,
                        operation.content
                    );
                    break;

                case MessageOperationType::Delete:
                    success = deleteMessageRequest(
                        operation.channelId,
                        operation.messageId
                    );
                    break;

                case MessageOperationType::SendEmbed:
                    success = sendEmbedRequest(
                        operation.channelId,
                        operation.content,
                        messageId
                    );
                    break;

                case MessageOperationType::SendComponents:
                    success = sendComponentsRequest(
                        operation.channelId,
                        operation.content,
                        messageId
                    );
                    break;
            }

            {
                std::lock_guard<std::mutex> lock(resultMutex_);

                messageOperationResults_.push_back(
                    MessageOperationResult{
                        operation.type,
                        success,
                        operation.channelId,
                        messageId
                    }
                );
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

        const std::wstring headers =
            L"Authorization: Bot " + tokenWide +
            L"\r\nContent-Type: application/json"
            L"\r\nAccept: application/json"
            L"\r\nUser-Agent: DiscordBridge/0.0.1"
            L"\r\n";

        const std::string body =
            "{\"content\":\"" +
            EscapeJson(AnsiToUtf8(message)) +
            "\"}";

        const std::wstring path =
            L"/api/v10/channels/" +
            channelWide +
            L"/messages";

        const HttpResponse response = httpClient_.post(
            L"discord.com",
            path,
            headers,
            body
        );

        if (!response.success) return false;

        if (!FindRootString(response.body, "id", messageId)) return false;

        return !messageId.empty();
    }

    bool DiscordClient::editMessageRequest(const std::string& channelId, const std::string& messageId, const std::string& content)
    {
        if (token_.empty() || channelId.empty() || messageId.empty() || content.empty()) return false;

        const std::wstring tokenWide = Utf8ToWide(token_);
        const std::wstring channelWide = Utf8ToWide(channelId);
        const std::wstring messageWide = Utf8ToWide(messageId);

        if (tokenWide.empty() || channelWide.empty() || messageWide.empty()) return false;

        const std::wstring headers =
            L"Authorization: Bot " + tokenWide +
            L"\r\nContent-Type: application/json"
            L"\r\nAccept: application/json"
            L"\r\nUser-Agent: DiscordBridge/0.0.1"
            L"\r\n";

        const std::string body =
            "{\"content\":\"" +
            EscapeJson(AnsiToUtf8(content)) +
            "\"}";

        const std::wstring path =
            L"/api/v10/channels/" +
            channelWide +
            L"/messages/" +
            messageWide;

        return httpClient_.patch(
            L"discord.com",
            path,
            headers,
            body
        ).success;
    }

    bool DiscordClient::deleteMessageRequest(const std::string& channelId, const std::string& messageId)
    {
        if (token_.empty() || channelId.empty() || messageId.empty()) return false;

        const std::wstring tokenWide = Utf8ToWide(token_);
        const std::wstring channelWide = Utf8ToWide(channelId);
        const std::wstring messageWide = Utf8ToWide(messageId);

        if (tokenWide.empty() || channelWide.empty() || messageWide.empty()) return false;

        const std::wstring headers =
            L"Authorization: Bot " + tokenWide +
            L"\r\nAccept: application/json"
            L"\r\nUser-Agent: DiscordBridge/0.0.1"
            L"\r\n";

        const std::wstring path =
            L"/api/v10/channels/" +
            channelWide +
            L"/messages/" +
            messageWide;

        return httpClient_.del(
            L"discord.com",
            path,
            headers
        ).success;
    }

    bool DiscordClient::sendEmbedRequest(const std::string& channelId, const std::string& embedJson, std::string& messageId)
    {
        messageId.clear();

        if (token_.empty() || channelId.empty() || embedJson.empty()) return false;

        const std::wstring tokenWide = Utf8ToWide(token_);
        const std::wstring channelWide = Utf8ToWide(channelId);

        if (tokenWide.empty() || channelWide.empty()) return false;

        const std::wstring headers =
            L"Authorization: Bot " + tokenWide +
            L"\r\nContent-Type: application/json"
            L"\r\nAccept: application/json"
            L"\r\nUser-Agent: DiscordBridge/0.0.1"
            L"\r\n";

        const std::string body =
            "{\"embeds\":[" +
            embedJson +
            "]}";

        const std::wstring path =
            L"/api/v10/channels/" +
            channelWide +
            L"/messages";

        const HttpResponse response = httpClient_.post(
            L"discord.com",
            path,
            headers,
            body
        );

        if (!response.success) return false;

        if (!FindRootString(response.body, "id", messageId)) return false;

        return !messageId.empty();
    }

    bool DiscordClient::sendComponentsRequest(const std::string& channelId, const std::string& componentsJson, std::string& messageId)
    {
        messageId.clear();

        if (token_.empty() || channelId.empty() || componentsJson.empty()) return false;

        const std::wstring tokenWide = Utf8ToWide(token_);
        const std::wstring channelWide = Utf8ToWide(channelId);

        if (tokenWide.empty() || channelWide.empty()) return false;

        const std::wstring headers =
            L"Authorization: Bot " + tokenWide +
            L"\r\nContent-Type: application/json"
            L"\r\nAccept: application/json"
            L"\r\nUser-Agent: DiscordBridge/0.0.1"
            L"\r\n";

        const std::string body = "{\"components\":[" + componentsJson + "]}";

        const std::wstring path =
            L"/api/v10/channels/" +
            channelWide +
            L"/messages";

        const HttpResponse response = httpClient_.post(
            L"discord.com",
            path,
            headers,
            body
        );

        if (!response.success) return false;

        if (!FindRootString(response.body, "id", messageId)) return false;

        return !messageId.empty();
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