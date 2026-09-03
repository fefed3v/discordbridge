#include "DiscordClient.hpp"
#include "../../Version.hpp"
#include "../../core/Limits.hpp"
#include "../../core/Metrics.hpp"
#include "../../core/Debug.hpp"
#include "../../security/InputValidator.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <utility>

#include <chrono>
#include <iostream>
#include <thread>

namespace DiscordBridge
{
    namespace
    {
        std::wstring Utf8ToWide(const std::string &value)
        {
#ifdef _WIN32
            if (value.empty())
                return {};
            const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
            if (size <= 0)
                return {};
            std::wstring result(static_cast<std::size_t>(size), L'\0');
            if (MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size) <= 0)
                return {};
            return result;
#else
            return std::wstring(value.begin(), value.end());
#endif
        }

        std::string AnsiToUtf8(const std::string &value)
        {
#ifdef _WIN32
            if (value.empty())
                return {};
            const int wideSize = MultiByteToWideChar(CP_ACP, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
            if (wideSize <= 0)
                return value;
            std::wstring wide(static_cast<std::size_t>(wideSize), L'\0');
            if (MultiByteToWideChar(CP_ACP, 0, value.data(), static_cast<int>(value.size()), wide.data(), wideSize) <= 0)
                return value;
            const int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wideSize, nullptr, 0, nullptr, nullptr);
            if (utf8Size <= 0)
                return value;
            std::string result(static_cast<std::size_t>(utf8Size), '\0');
            if (WideCharToMultiByte(CP_UTF8, 0, wide.data(), wideSize, result.data(), utf8Size, nullptr, nullptr) <= 0)
                return value;
            return result;
#else
            return value;
#endif
        }

        std::string EscapeJson(const std::string &value)
        {
            std::string result;
            result.reserve(value.size());

            for (const char character : value)
            {
                switch (character)
                {
                case '"':
                    result += "\\\"";
                    break;
                case '\\':
                    result += "\\\\";
                    break;
                case '\n':
                    result += "\\n";
                    break;
                case '\r':
                    result += "\\r";
                    break;
                case '\t':
                    result += "\\t";
                    break;
                case '\b':
                    result += "\\b";
                    break;
                case '\f':
                    result += "\\f";
                    break;
                default:
                    result.push_back(character);
                    break;
                }
            }

            return result;
        }

        bool FindRootString(const std::string &json, const std::string &key, std::string &value)
        {
            std::size_t position = 0;

            while (position < json.size() && std::isspace(static_cast<unsigned char>(json[position])))
                ++position;

            if (position >= json.size() || json[position] != '{')
                return false;

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
                        if (escaped)
                            escaped = false;
                        else if (character == '\\')
                            escaped = true;
                        else if (character == '"')
                            insideString = false;
                    }
                    else
                    {
                        if (character == '"')
                            insideString = true;
                        else if (character == '{' || character == '[')
                            ++depth;
                        else if (character == '}' || character == ']')
                            --depth;
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

                if (position >= json.size() || json[position] == '}')
                    break;

                if (json[position] != '"')
                    return false;

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

                    if (character == '"')
                        break;

                    currentKey.push_back(character);
                }

                while (position < json.size() && std::isspace(static_cast<unsigned char>(json[position])))
                    ++position;

                if (position >= json.size() || json[position] != ':')
                    return false;

                ++position;

                while (position < json.size() && std::isspace(static_cast<unsigned char>(json[position])))
                    ++position;

                if (position >= json.size())
                    return false;

                if (currentKey == key)
                {
                    if (json[position] != '"')
                        return false;

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
                            case '"':
                                result.push_back('"');
                                break;
                            case '\\':
                                result.push_back('\\');
                                break;
                            case '/':
                                result.push_back('/');
                                break;
                            case 'b':
                                result.push_back('\b');
                                break;
                            case 'f':
                                result.push_back('\f');
                                break;
                            case 'n':
                                result.push_back('\n');
                                break;
                            case 'r':
                                result.push_back('\r');
                                break;
                            case 't':
                                result.push_back('\t');
                                break;
                            default:
                                result.push_back(character);
                                break;
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

                        if (character == '"')
                            break;
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
                            if (nestedEscaped)
                                nestedEscaped = false;
                            else if (character == '\\')
                                nestedEscaped = true;
                            else if (character == '"')
                                nestedString = false;

                            continue;
                        }

                        if (character == '"')
                        {
                            nestedString = true;
                            continue;
                        }

                        if (character == opening)
                            ++nestedDepth;
                        else if (character == closing)
                            --nestedDepth;
                    }
                }
                else
                {
                    while (position < json.size() && json[position] != ',' && json[position] != '}')
                        ++position;
                }
            }

            return false;
        }

        long RetryAfterMs(const std::string &body)
        {
            const std::string key = "\"retry_after\"";
            std::size_t position = body.find(key);
            if (position == std::string::npos) return 1000;
            position = body.find(':', position + key.size());
            if (position == std::string::npos) return 1000;
            ++position;
            while (position < body.size() && std::isspace(static_cast<unsigned char>(body[position]))) ++position;
            const char *start = body.c_str() + position;
            char *end = nullptr;
            const double seconds = std::strtod(start, &end);
            if (end == start || seconds < 0.0) return 1000;
            long milliseconds = static_cast<long>(seconds * 1000.0) + 100;
            if (milliseconds < 100) milliseconds = 100;
            if (milliseconds > 60000) milliseconds = 60000;
            return milliseconds;
        }

        template <typename Request>
        HttpResponse PerformDiscordRequest(Request &&request)
        {
            HttpResponse response;
            for (int attempt = 0; attempt < 4; ++attempt)
            {
                response = request();
                if (response.statusCode == 429)
                {
                    ++GlobalMetrics().rateLimitedRequests;
                    const long waitMs = RetryAfterMs(response.body);
                    DebugLog::warn("RateLimit", "Discord returned 429; waiting " + std::to_string(waitMs) + "ms");
                    std::this_thread::sleep_for(std::chrono::milliseconds(waitMs));
                    continue;
                }
                if (response.statusCode >= 500 && response.statusCode <= 599 && attempt < 3)
                {
                    ++GlobalMetrics().httpServerErrors;
                    const long waitMs = 250L * (1L << attempt);
                    DebugLog::warn("HTTP", "Discord server error " + std::to_string(response.statusCode) + "; retrying");
                    std::this_thread::sleep_for(std::chrono::milliseconds(waitMs));
                    continue;
                }
                return response;
            }
            return response;
        }
    }

    DiscordClient::~DiscordClient()
    {
        shutdown();
    }

    bool DiscordClient::initialize(const std::string &token)
    {
        if (initialized_ || token.empty())
            return false;

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
            L"\r\nUser-Agent: DiscordBridge/1.0.0"
            L"\r\n";

        const HttpResponse response = httpClient_.get(
            L"discord.com",
            L"/api/v10/gateway/bot",
            headers);

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

        if (!interactionHttpClient_.open())
        {
            gatewayInfo_ = GatewayInfo{};
            token_.clear();
            return false;
        }

        if (!gateway_.connect(gatewayInfo_, token_))
        {
            gatewayInfo_ = GatewayInfo{};
            token_.clear();

            return false;
        }

        restRunning_ = true;
        interactionRunning_ = true;

        try
        {
            restThread_ = std::thread(&DiscordClient::restLoop, this);
            interactionThread_ = std::thread(&DiscordClient::interactionLoop, this);
        }
        catch (...)
        {
            restRunning_ = false;
            interactionRunning_ = false;
            outgoingCondition_.notify_all();
            interactionCondition_.notify_all();

            if (restThread_.joinable())
                restThread_.join();
            if (interactionThread_.joinable())
                interactionThread_.join();

            gateway_.disconnect();

            gatewayInfo_ = GatewayInfo{};
            token_.clear();

            return false;
        }

        initialized_ = true;
        connected_ = false;
        reconnectFailures_ = 0;
        nextReconnectAttempt_ = {};
        DebugLog::info("Client", "Discord client initialized");

        return true;
    }

    void DiscordClient::shutdown()
    {
        initialized_ = false;
        connected_ = false;

        restRunning_ = false;
        interactionRunning_ = false;
        outgoingCondition_.notify_all();
        interactionCondition_.notify_all();

        if (restThread_.joinable() && restThread_.get_id() != std::this_thread::get_id())
            restThread_.join();
        if (interactionThread_.joinable() && interactionThread_.get_id() != std::this_thread::get_id())
            interactionThread_.join();

        {
            std::lock_guard<std::mutex> lock(outgoingMutex_);
            messageOperations_.clear();
        }

        {
            std::lock_guard<std::mutex> lock(resultMutex_);
            messageOperationResults_.clear();
            guildOperationResults_.clear();
        }

        {
            std::lock_guard<std::mutex> lock(interactionMutex_);
            interactionOperations_.clear();
        }

        {
            std::lock_guard<std::mutex> lock(interactionResponseMutex_);
            interactionResponses_.clear();
        }

        gateway_.disconnect();

        gatewayInfo_ = GatewayInfo{};
        dataStore_.clear();
        token_.clear();
        reconnectFailures_ = 0;
        nextReconnectAttempt_ = {};
    }

    void DiscordClient::process()
    {
        if (!initialized_)
            return;

        connected_ = gateway_.isReady();
        if (connected_)
        {
            reconnectFailures_ = 0;
            nextReconnectAttempt_ = {};
            return;
        }

        if (token_.empty() || !gatewayInfo_.isValid())
            return;

        const auto now = std::chrono::steady_clock::now();
        if (nextReconnectAttempt_.time_since_epoch().count() != 0 && now < nextReconnectAttempt_)
            return;

        ++GlobalMetrics().reconnectAttempts;
        DebugLog::info("Gateway", "Connection lost; attempting reconnect");
        if (gateway_.reconnect(gatewayInfo_, token_))
        {
            reconnectFailures_ = 0;
            ++GlobalMetrics().reconnectSuccesses;
            nextReconnectAttempt_ = now + std::chrono::seconds(5);
            DebugLog::info("Gateway", "Reconnect transport established");
            return;
        }

        ++reconnectFailures_;
        const unsigned int exponent = reconnectFailures_ > 4 ? 4 : reconnectFailures_;
        const unsigned int delaySeconds = 5u * (1u << exponent);
        nextReconnectAttempt_ = now + std::chrono::seconds(delaySeconds > 60u ? 60u : delaySeconds);
        DebugLog::warn("Gateway", "Reconnect failed; retry scheduled");
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

    bool DiscordClient::consumeMessageCreateEvent(std::string &userId, std::string &channelId, std::string &message)
    {
        return gateway_.consumeMessageCreateEvent(userId, channelId, message);
    }

    bool DiscordClient::consumeGuildMemberAddEvent(std::string &guildId, std::string &userId)
    {
        return gateway_.consumeGuildMemberAddEvent(guildId, userId);
    }

    bool DiscordClient::consumeGuildMemberRemoveEvent(std::string &guildId, std::string &userId)
    {
        return gateway_.consumeGuildMemberRemoveEvent(guildId, userId);
    }

    bool DiscordClient::consumeButtonClickEvent(std::string &interactionId, std::string &interactionToken, std::string &userId, std::string &guildId, std::string &channelId, std::string &customId)
    {
        return gateway_.consumeButtonClickEvent(interactionId, interactionToken, userId, guildId, channelId, customId);
    }

    bool DiscordClient::consumeSelectMenuEvent(std::string &interactionId, std::string &interactionToken, std::string &userId, std::string &guildId, std::string &channelId, std::string &customId, std::vector<std::string> &values)
    {
        return gateway_.consumeSelectMenuEvent(interactionId, interactionToken, userId, guildId, channelId, customId, values);
    }

    bool DiscordClient::consumeModalEvent(std::string &interactionId, std::string &interactionToken, std::string &userId, std::string &guildId, std::string &channelId, std::string &customId, std::vector<std::pair<std::string, std::string>> &values)
    {
        return gateway_.consumeModalEvent(interactionId, interactionToken, userId, guildId, channelId, customId, values);
    }

    bool DiscordClient::consumeSlashCommandEvent(std::string &interactionId, std::string &interactionToken, std::string &userId, std::string &guildId, std::string &channelId, std::string &commandName, std::vector<std::pair<std::string, std::string>> &options)
    {
        return gateway_.consumeSlashCommandEvent(interactionId, interactionToken, userId, guildId, channelId, commandName, options);
    }
    bool DiscordClient::consumeAutocompleteEvent(std::string &interactionId, std::string &interactionToken, std::string &userId, std::string &guildId, std::string &channelId, std::string &commandName, std::vector<std::pair<std::string, std::string>> &options)
    {
        return gateway_.consumeAutocompleteEvent(interactionId, interactionToken, userId, guildId, channelId, commandName, options);
    }

    bool DiscordClient::respondAutocomplete(const std::string &interactionId, const std::string &interactionToken, const std::string &choicesJson)
    {
        if (!initialized_ || interactionId.empty() || interactionToken.empty() || choicesJson.empty() || !reserveInteractionResponse(interactionId))
            return false;
        if (!enqueueInteractionOperation({interactionId, interactionToken, "{\"type\":8,\"data\":{\"choices\":" + choicesJson + "}}"}, true))
        {
            releaseInteractionResponse(interactionId);
            return false;
        }
        return true;
    }

    bool DiscordClient::reserveInteractionResponse(const std::string &interactionId)
    {
        if (interactionId.empty()) return false;
        const auto now = std::chrono::steady_clock::now();
        const auto expiry = std::chrono::minutes(20);
        std::lock_guard<std::mutex> lock(interactionResponseMutex_);
        for (auto it = interactionResponses_.begin(); it != interactionResponses_.end();)
        {
            if (now - it->second >= expiry) it = interactionResponses_.erase(it);
            else ++it;
        }
        return interactionResponses_.emplace(interactionId, now).second;
    }

    void DiscordClient::releaseInteractionResponse(const std::string &interactionId)
    {
        std::lock_guard<std::mutex> lock(interactionResponseMutex_);
        interactionResponses_.erase(interactionId);
    }

    bool DiscordClient::acknowledgeInteractionAsync(const std::string &id, const std::string &token)
    {
        if (!initialized_ || id.empty() || token.empty()) return false;
        {
            std::lock_guard<std::mutex> lock(interactionResponseMutex_);
            if (interactionResponses_.find(id) != interactionResponses_.end()) return true;
        }
        if (!reserveInteractionResponse(id)) return true;
        if (!enqueueInteractionOperation({id, token, "{\"type\":6}"}, true))
        {
            releaseInteractionResponse(id);
            return false;
        }
        return true;
    }

    bool DiscordClient::deferInteractionAsync(const std::string &id, const std::string &token, bool ephemeral)
    {
        if (!initialized_ || id.empty() || token.empty()) return false;
        {
            std::lock_guard<std::mutex> lock(interactionResponseMutex_);
            if (interactionResponses_.find(id) != interactionResponses_.end()) return true;
        }
        if (!reserveInteractionResponse(id)) return true;
        std::string body = "{\"type\":5,\"data\":{";
        if (ephemeral) body += "\"flags\":64";
        body += "}}";
        if (!enqueueInteractionOperation({id, token, std::move(body)}, true))
        {
            releaseInteractionResponse(id);
            return false;
        }
        return true;
    }

    bool DiscordClient::respondInteraction(const std::string &id, const std::string &token, const std::string &content, bool ephemeral)
    {
        if (!initialized_ || id.empty() || token.empty() || content.empty() || content.size() > 2000 || !reserveInteractionResponse(id)) return false;
        std::string body = "{\"type\":4,\"data\":{\"content\":\"" + EscapeJson(AnsiToUtf8(content)) + "\"";
        if (ephemeral) body += ",\"flags\":64";
        body += "}}";
        if (!enqueueInteractionOperation({id, token, std::move(body)}, true))
        {
            releaseInteractionResponse(id);
            return false;
        }
        return true;
    }

    bool DiscordClient::showModal(const std::string &id, const std::string &token, const std::string &modalJson)
    {
        if (!initialized_ || id.empty() || token.empty() || modalJson.empty() || !reserveInteractionResponse(id)) return false;
        if (!enqueueInteractionOperation({id, token, "{\"type\":9,\"data\":" + modalJson + "}"}, true))
        {
            releaseInteractionResponse(id);
            return false;
        }
        return true;
    }

    bool DiscordClient::consumeMessageSentEvent(bool &success, std::string &channelId, std::string &messageId)
    {
        return consumeMessageOperationResult(
            MessageOperationType::Send,
            success,
            channelId,
            messageId);
    }

    bool DiscordClient::consumeMessageEditedEvent(bool &success, std::string &channelId, std::string &messageId)
    {
        return consumeMessageOperationResult(
            MessageOperationType::Edit,
            success,
            channelId,
            messageId);
    }

    bool DiscordClient::consumeMessageDeletedEvent(bool &success, std::string &channelId, std::string &messageId)
    {
        return consumeMessageOperationResult(
            MessageOperationType::Delete,
            success,
            channelId,
            messageId);
    }

    bool DiscordClient::consumeEmbedSentEvent(bool &success, std::string &channelId, std::string &messageId)
    {
        return consumeMessageOperationResult(
            MessageOperationType::SendEmbed,
            success,
            channelId,
            messageId);
    }

    bool DiscordClient::consumeComponentsSentEvent(bool &success, std::string &channelId, std::string &messageId)
    {
        return consumeMessageOperationResult(
            MessageOperationType::SendComponents,
            success,
            channelId,
            messageId);
    }

    bool DiscordClient::consumeV2SentEvent(bool &success, std::string &channelId, std::string &messageId)
    {
        return consumeMessageOperationResult(MessageOperationType::SendV2, success, channelId, messageId);
    }

    bool DiscordClient::consumeV2EditedEvent(bool &success, std::string &channelId, std::string &messageId)
    {
        return consumeMessageOperationResult(MessageOperationType::EditV2, success, channelId, messageId);
    }

    bool DiscordClient::consumeMessageOperationResult(MessageOperationType type, bool &success, std::string &channelId, std::string &messageId)
    {
        std::lock_guard<std::mutex> lock(resultMutex_);

        for (auto iterator = messageOperationResults_.begin(); iterator != messageOperationResults_.end(); ++iterator)
        {
            if (iterator->type != type)
                continue;

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

    bool DiscordClient::setActivity(int type, const std::string &name, const std::string &state, const std::string &url)
    {
        return gateway_.setActivity(type, name, state, url);
    }

    bool DiscordClient::clearActivity()
    {
        return gateway_.clearActivity();
    }

    bool DiscordClient::setPresence(int status, int activityType, const std::string &name, const std::string &state, const std::string &url, bool afk)
    {
        return gateway_.setPresence(
            status,
            activityType,
            name,
            state,
            url,
            afk);
    }

    bool DiscordClient::sendMessage(const std::string &channelId, const std::string &message)
    {
        if (!initialized_ || !restRunning_)
            return false;
        if (!InputValidator::snowflake(channelId) || !InputValidator::message(message))
            return false;
        if (message.size() > 2000)
            return false;

        return enqueueMessageOperation(
            MessageOperation{
                MessageOperationType::Send,
                channelId,
                {},
                message});
    }

    bool DiscordClient::editMessage(const std::string &channelId, const std::string &messageId, const std::string &content)
    {
        if (!initialized_ || !restRunning_)
            return false;
        if (!InputValidator::snowflake(channelId) || !InputValidator::snowflake(messageId) || !InputValidator::message(content))
            return false;
        if (content.size() > 2000)
            return false;

        return enqueueMessageOperation(
            MessageOperation{
                MessageOperationType::Edit,
                channelId,
                messageId,
                content});
    }

    bool DiscordClient::deleteMessage(const std::string &channelId, const std::string &messageId)
    {
        if (!initialized_ || !restRunning_)
            return false;
        if (!InputValidator::snowflake(channelId) || !InputValidator::snowflake(messageId))
            return false;

        return enqueueMessageOperation(
            MessageOperation{
                MessageOperationType::Delete,
                channelId,
                messageId,
                {}});
    }

    bool DiscordClient::sendEmbed(const std::string &channelId, const std::string &embedJson)
    {
        if (!initialized_ || !restRunning_)
            return false;
        if (!InputValidator::snowflake(channelId) || embedJson.empty())
            return false;

        return enqueueMessageOperation(
            MessageOperation{
                MessageOperationType::SendEmbed,
                channelId,
                {},
                embedJson});
    }

    bool DiscordClient::sendComponents(const std::string &channelId, const std::string &componentsJson)
    {
        if (!initialized_ || !restRunning_)
            return false;
        if (!InputValidator::snowflake(channelId) || componentsJson.empty())
            return false;

        return enqueueMessageOperation(
            MessageOperation{
                MessageOperationType::SendComponents,
                channelId,
                {},
                componentsJson});
    }

    bool DiscordClient::sendV2(const std::string &channelId, const std::string &messageJson)
    {
        if (!initialized_ || !restRunning_ || !InputValidator::snowflake(channelId) || messageJson.empty())
            return false;
        return enqueueMessageOperation({MessageOperationType::SendV2, channelId, {}, messageJson});
    }

    bool DiscordClient::editV2(const std::string &channelId, const std::string &messageId, const std::string &messageJson)
    {
        if (!initialized_ || !restRunning_ || !InputValidator::snowflake(channelId) || !InputValidator::snowflake(messageId) || messageJson.empty())
            return false;
        return enqueueMessageOperation({MessageOperationType::EditV2, channelId, messageId, messageJson});
    }

    bool DiscordClient::respondV2(const std::string &interactionId, const std::string &interactionToken, const std::string &componentsJson, bool ephemeral)
    {
        if (!initialized_ || interactionId.empty() || interactionToken.empty() || componentsJson.empty())
            return false;
        if (!reserveInteractionResponse(interactionId)) return false;
        const int flags = 32768 | (ephemeral ? 64 : 0);
        const std::string body = "{\"type\":4,\"data\":{\"flags\":" + std::to_string(flags) + ",\"components\":" + componentsJson + "}}";
        if (!enqueueInteractionOperation({interactionId, interactionToken, body}, true))
        {
            releaseInteractionResponse(interactionId);
            return false;
        }
        return true;
    }

    bool DiscordClient::fetchGuild(const std::string &guildId)
    {
        return initialized_ && restRunning_ && InputValidator::snowflake(guildId) && enqueueMessageOperation({MessageOperationType::FetchGuild, guildId, {}, {}});
    }

    bool DiscordClient::fetchChannel(const std::string &channelId)
    {
        return initialized_ && restRunning_ && InputValidator::snowflake(channelId) && enqueueMessageOperation({MessageOperationType::FetchChannel, {}, channelId, {}});
    }

    bool DiscordClient::fetchRole(const std::string &guildId, const std::string &roleId)
    {
        return initialized_ && restRunning_ && InputValidator::snowflake(guildId) && InputValidator::snowflake(roleId) && enqueueMessageOperation({MessageOperationType::FetchRole, guildId, roleId, {}});
    }

    bool DiscordClient::fetchMember(const std::string &guildId, const std::string &userId)
    {
        return initialized_ && restRunning_ && InputValidator::snowflake(guildId) && InputValidator::snowflake(userId) && enqueueMessageOperation({MessageOperationType::FetchMember, guildId, userId, {}});
    }

    bool DiscordClient::fetchUser(const std::string &userId)
    {
        return initialized_ && restRunning_ && InputValidator::snowflake(userId) && enqueueMessageOperation({MessageOperationType::FetchUser, {}, userId, {}});
    }

    bool DiscordClient::setMemberNick(const std::string &guildId, const std::string &userId, const std::string &nickname)
    {
        if (!initialized_ || !restRunning_ || !InputValidator::snowflake(guildId) || !InputValidator::snowflake(userId) || nickname.size() > 32) return false;
        const std::string body = "{\"nick\":\"" + EscapeJson(AnsiToUtf8(nickname)) + "\"}";
        return enqueueMessageOperation({MessageOperationType::SetMemberNick, guildId, userId, body});
    }

    bool DiscordClient::setBotUsername(const std::string &username)
    {
        if (!initialized_ || !restRunning_ || username.size() < 2 || username.size() > 32) return false;
        return enqueueMessageOperation({MessageOperationType::SetBotProfile, "username", {}, "{\"username\":\"" + EscapeJson(AnsiToUtf8(username)) + "\"}"});
    }

    bool DiscordClient::setBotAvatar(const std::string &imageData)
    {
        if (!initialized_ || !restRunning_ || imageData.empty()) return false;
        return enqueueMessageOperation({MessageOperationType::SetBotProfile, "avatar", {}, "{\"avatar\":\"" + EscapeJson(imageData) + "\"}"});
    }

    bool DiscordClient::setBotBanner(const std::string &imageData)
    {
        if (!initialized_ || !restRunning_ || imageData.empty()) return false;
        return enqueueMessageOperation({MessageOperationType::SetBotProfile, "banner", {}, "{\"banner\":\"" + EscapeJson(imageData) + "\"}"});
    }

    bool DiscordClient::isCurrentBot(const std::string &userId) const
    {
        return InputValidator::snowflake(userId) && userId == gateway_.getApplicationId();
    }

    DiscordDataStore &DiscordClient::getDataStore() { return dataStore_; }
    const DiscordDataStore &DiscordClient::getDataStore() const { return dataStore_; }

    bool DiscordClient::createChannel(const std::string &guildId, const std::string &name, int type)
    {
        if (!initialized_ || !restRunning_ || !InputValidator::snowflake(guildId) || name.empty() || name.size() > 100 || type < 0)
            return false;
        const std::string body = "{\"name\":\"" + EscapeJson(AnsiToUtf8(name)) + "\",\"type\":" + std::to_string(type) + "}";
        return enqueueMessageOperation({MessageOperationType::CreateChannel, guildId, {}, body});
    }

    bool DiscordClient::deleteChannel(const std::string &channelId)
    {
        if (!initialized_ || !restRunning_ || !InputValidator::snowflake(channelId))
            return false;
        return enqueueMessageOperation({MessageOperationType::DeleteChannel, {}, channelId, {}});
    }

    bool DiscordClient::createRole(const std::string &guildId, const std::string &name, int color, bool hoist, bool mentionable)
    {
        if (!initialized_ || !restRunning_ || !InputValidator::snowflake(guildId) || name.empty() || name.size() > 100 || color < 0 || color > 0xFFFFFF)
            return false;
        const std::string body = "{\"name\":\"" + EscapeJson(AnsiToUtf8(name)) + "\",\"color\":" + std::to_string(color) +
                                 ",\"hoist\":" + (hoist ? "true" : "false") + ",\"mentionable\":" + (mentionable ? "true" : "false") + "}";
        return enqueueMessageOperation({MessageOperationType::CreateRole, guildId, {}, body});
    }

    bool DiscordClient::deleteRole(const std::string &guildId, const std::string &roleId)
    {
        if (!initialized_ || !restRunning_ || !InputValidator::snowflake(guildId) || !InputValidator::snowflake(roleId))
            return false;
        return enqueueMessageOperation({MessageOperationType::DeleteRole, guildId, roleId, {}});
    }

    bool DiscordClient::addMemberRole(const std::string &guildId, const std::string &userId, const std::string &roleId)
    {
        if (!initialized_ || !restRunning_ || !InputValidator::snowflake(guildId) || !InputValidator::snowflake(userId) || !InputValidator::snowflake(roleId))
            return false;
        return enqueueMessageOperation({MessageOperationType::AddMemberRole, guildId, userId, roleId});
    }

    bool DiscordClient::removeMemberRole(const std::string &guildId, const std::string &userId, const std::string &roleId)
    {
        if (!initialized_ || !restRunning_ || !InputValidator::snowflake(guildId) || !InputValidator::snowflake(userId) || !InputValidator::snowflake(roleId))
            return false;
        return enqueueMessageOperation({MessageOperationType::RemoveMemberRole, guildId, userId, roleId});
    }

    bool DiscordClient::kickMember(const std::string &guildId, const std::string &userId)
    {
        if (!initialized_ || !restRunning_ || !InputValidator::snowflake(guildId) || !InputValidator::snowflake(userId))
            return false;
        return enqueueMessageOperation({MessageOperationType::KickMember, guildId, userId, {}});
    }

    bool DiscordClient::banMember(const std::string &guildId, const std::string &userId, int deleteMessageSeconds)
    {
        if (!initialized_ || !restRunning_ || !InputValidator::snowflake(guildId) || !InputValidator::snowflake(userId) || deleteMessageSeconds < 0 || deleteMessageSeconds > 604800)
            return false;
        const std::string body = "{\"delete_message_seconds\":" + std::to_string(deleteMessageSeconds) + "}";
        return enqueueMessageOperation({MessageOperationType::BanMember, guildId, userId, body});
    }

    bool DiscordClient::unbanMember(const std::string &guildId, const std::string &userId)
    {
        if (!initialized_ || !restRunning_ || !InputValidator::snowflake(guildId) || !InputValidator::snowflake(userId))
            return false;
        return enqueueMessageOperation({MessageOperationType::UnbanMember, guildId, userId, {}});
    }

    bool DiscordClient::deployCommands(const std::string &guildId, const std::string &commandsJson)
    {
        const std::string applicationId = gateway_.getApplicationId();
        if (!initialized_ || !restRunning_ || !InputValidator::snowflake(guildId) || commandsJson.empty() || !InputValidator::snowflake(applicationId))
            return false;
        return enqueueMessageOperation({MessageOperationType::DeployCommands, guildId, applicationId, commandsJson}, true);
    }

    bool DiscordClient::consumeGuildOperationResultInternal(MessageOperationType type, bool &success, std::string &guildId, std::string &targetId)
    {
        std::lock_guard<std::mutex> lock(resultMutex_);
        for (auto it = guildOperationResults_.begin(); it != guildOperationResults_.end(); ++it)
        {
            if (it->type != type)
                continue;
            success = it->success;
            guildId = std::move(it->guildId);
            targetId = std::move(it->targetId);
            guildOperationResults_.erase(it);
            return true;
        }
        return false;
    }

    bool DiscordClient::consumeChannelCreatedEvent(bool &s, std::string &g, std::string &id) { return consumeGuildOperationResultInternal(MessageOperationType::CreateChannel, s, g, id); }
    bool DiscordClient::consumeChannelDeletedEvent(bool &s, std::string &g, std::string &id) { return consumeGuildOperationResultInternal(MessageOperationType::DeleteChannel, s, g, id); }
    bool DiscordClient::consumeRoleCreatedEvent(bool &s, std::string &g, std::string &id) { return consumeGuildOperationResultInternal(MessageOperationType::CreateRole, s, g, id); }
    bool DiscordClient::consumeRoleDeletedEvent(bool &s, std::string &g, std::string &id) { return consumeGuildOperationResultInternal(MessageOperationType::DeleteRole, s, g, id); }
    bool DiscordClient::consumeMemberRoleAddedEvent(bool &s, std::string &g, std::string &id) { return consumeGuildOperationResultInternal(MessageOperationType::AddMemberRole, s, g, id); }
    bool DiscordClient::consumeMemberRoleRemovedEvent(bool &s, std::string &g, std::string &id) { return consumeGuildOperationResultInternal(MessageOperationType::RemoveMemberRole, s, g, id); }
    bool DiscordClient::consumeMemberKickedEvent(bool &s, std::string &g, std::string &id) { return consumeGuildOperationResultInternal(MessageOperationType::KickMember, s, g, id); }
    bool DiscordClient::consumeMemberBannedEvent(bool &s, std::string &g, std::string &id) { return consumeGuildOperationResultInternal(MessageOperationType::BanMember, s, g, id); }
    bool DiscordClient::consumeMemberUnbannedEvent(bool &s, std::string &g, std::string &id) { return consumeGuildOperationResultInternal(MessageOperationType::UnbanMember, s, g, id); }
    bool DiscordClient::consumeMemberNickSetEvent(bool &s, std::string &g, std::string &id) { return consumeGuildOperationResultInternal(MessageOperationType::SetMemberNick, s, g, id); }
    bool DiscordClient::consumeBotProfileSetEvent(bool &s, std::string &field) { std::string ignored; return consumeGuildOperationResultInternal(MessageOperationType::SetBotProfile, s, field, ignored); }
    bool DiscordClient::consumeCommandsDeployedEvent(bool &s, std::string &g)
    {
        std::string id;
        return consumeGuildOperationResultInternal(MessageOperationType::DeployCommands, s, g, id);
    }
    bool DiscordClient::consumeGuildFetchedEvent(bool &s, std::string &g)
    {
        std::string id;
        return consumeGuildOperationResultInternal(MessageOperationType::FetchGuild, s, g, id);
    }
    bool DiscordClient::consumeChannelFetchedEvent(bool &s, std::string &c)
    {
        std::string g;
        return consumeGuildOperationResultInternal(MessageOperationType::FetchChannel, s, g, c);
    }
    bool DiscordClient::consumeRoleFetchedEvent(bool &s, std::string &g, std::string &r) { return consumeGuildOperationResultInternal(MessageOperationType::FetchRole, s, g, r); }
    bool DiscordClient::consumeMemberFetchedEvent(bool &s, std::string &g, std::string &u) { return consumeGuildOperationResultInternal(MessageOperationType::FetchMember, s, g, u); }
    bool DiscordClient::consumeUserFetchedEvent(bool &s, std::string &u)
    {
        std::string g;
        return consumeGuildOperationResultInternal(MessageOperationType::FetchUser, s, g, u);
    }

    bool DiscordClient::enqueueMessageOperation(MessageOperation operation, bool prioritize)
    {
        if (!restRunning_)
            return false;

        {
            std::lock_guard<std::mutex> lock(outgoingMutex_);

            if (!restRunning_)
                return false;

            if (messageOperations_.size() >= Limits::MaxRestQueue)
            {
                ++GlobalMetrics().droppedRestRequests;
                return false;
            }

            if (prioritize)
                messageOperations_.push_front(std::move(operation));
            else
                messageOperations_.push_back(std::move(operation));
        }

        outgoingCondition_.notify_one();
        return true;
    }

    bool DiscordClient::acknowledgeInteraction(const std::string &interactionId, const std::string &interactionToken)
    {
        return sendInteractionCallback(interactionId, interactionToken, "{\"type\":6}");
    }

    bool DiscordClient::enqueueInteractionOperation(InteractionOperation operation, bool prioritize)
    {
        if (!interactionRunning_ || operation.interactionId.empty() || operation.interactionToken.empty() || operation.body.empty())
            return false;

        {
            std::lock_guard<std::mutex> lock(interactionMutex_);
            if (!interactionRunning_)
                return false;

            if (interactionOperations_.size() >= Limits::MaxInteractionQueue)
            {
                ++GlobalMetrics().droppedInteractionRequests;
                return false;
            }
            if (prioritize)
                interactionOperations_.push_front(std::move(operation));
            else
                interactionOperations_.push_back(std::move(operation));
        }

        interactionCondition_.notify_one();
        return true;
    }

    bool DiscordClient::sendInteractionCallback(const std::string &interactionId, const std::string &interactionToken, const std::string &body)
    {
        if (interactionId.empty() || interactionToken.empty() || body.empty())
            return false;

        const std::wstring interactionWide = Utf8ToWide(interactionId);
        const std::wstring tokenWide = Utf8ToWide(interactionToken);
        if (interactionWide.empty() || tokenWide.empty())
            return false;

        const std::wstring headers = L"Content-Type: application/json\r\nAccept: application/json\r\nUser-Agent: DiscordBridge/1.0.0\r\n";
        const std::wstring path = L"/api/v10/interactions/" + interactionWide + L"/" + tokenWide + L"/callback";
        const HttpResponse response = PerformDiscordRequest([&]() { return interactionHttpClient_.post(L"discord.com", path, headers, body); });
        if (!response.success)
        {
            std::printf("[DiscordBridge]: Interaction HTTP falhou | Status: %lu | Body: %s\n", response.statusCode, response.body.empty() ? "<vazio>" : response.body.c_str());
        }
        return response.success;
    }

    void DiscordClient::interactionLoop()
    {
        while (interactionRunning_)
        {
            InteractionOperation operation;

            {
                std::unique_lock<std::mutex> lock(interactionMutex_);
                interactionCondition_.wait(lock, [this]
                                           { return !interactionRunning_ || !interactionOperations_.empty(); });
                // Shutdown is cancellation, not queue draining. Network calls may take
                // several seconds, so draining here can stall server/plugin unload.
                if (!interactionRunning_)
                    break;
                operation = std::move(interactionOperations_.front());
                interactionOperations_.pop_front();
            }

            sendInteractionCallback(operation.interactionId, operation.interactionToken, operation.body);
        }
    }

    void DiscordClient::restLoop()
    {
        while (restRunning_)
        {
            MessageOperation operation;

            {
                std::unique_lock<std::mutex> lock(outgoingMutex_);

                outgoingCondition_.wait(lock, [this]()
                                        { return !restRunning_.load() || !messageOperations_.empty(); });

                // Stop immediately on shutdown. Pending operations are discarded by
                // shutdown() after the worker exits, avoiding long unload stalls.
                if (!restRunning_)
                    break;

                if (messageOperations_.empty())
                    continue;

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
                    messageId);
                break;

            case MessageOperationType::Edit:
                success = editMessageRequest(
                    operation.channelId,
                    operation.messageId,
                    operation.content);
                break;

            case MessageOperationType::Delete:
                success = deleteMessageRequest(
                    operation.channelId,
                    operation.messageId);
                break;

            case MessageOperationType::SendEmbed:
                success = sendEmbedRequest(
                    operation.channelId,
                    operation.content,
                    messageId);
                break;

            case MessageOperationType::SendComponents:
                success = sendComponentsRequest(
                    operation.channelId,
                    operation.content,
                    messageId);
                break;

            case MessageOperationType::SendV2:
                success = sendV2Request(operation.channelId, operation.content, messageId);
                break;

            case MessageOperationType::EditV2:
                success = editV2Request(operation.channelId, operation.messageId, operation.content);
                break;

            case MessageOperationType::CreateChannel:
            case MessageOperationType::DeleteChannel:
            case MessageOperationType::CreateRole:
            case MessageOperationType::DeleteRole:
            case MessageOperationType::AddMemberRole:
            case MessageOperationType::RemoveMemberRole:
            case MessageOperationType::KickMember:
            case MessageOperationType::BanMember:
            case MessageOperationType::UnbanMember:
            case MessageOperationType::DeployCommands:
            case MessageOperationType::FetchGuild:
            case MessageOperationType::FetchChannel:
            case MessageOperationType::FetchRole:
            case MessageOperationType::FetchMember:
            case MessageOperationType::FetchUser:
            case MessageOperationType::SetMemberNick:
            case MessageOperationType::SetBotProfile:
            {
                const std::wstring tokenWide = Utf8ToWide(token_);
                const std::wstring guildWide = Utf8ToWide(operation.channelId);
                const std::wstring targetWide = Utf8ToWide(operation.messageId);
                const std::wstring extraWide = Utf8ToWide(operation.content);
                const std::wstring headers = L"Authorization: Bot " + tokenWide + L"\r\nAccept: application/json\r\nUser-Agent: DiscordBridge/1.0.0\r\n";
                const std::wstring jsonHeaders = headers + L"Content-Type: application/json\r\n";
                HttpResponse response;
                std::string resultId = operation.messageId;

                if (operation.type == MessageOperationType::DeployCommands)
                {
                    response = PerformDiscordRequest([&]() { return httpClient_.put(L"discord.com", L"/api/v10/applications/" + targetWide + L"/guilds/" + guildWide + L"/commands", jsonHeaders, operation.content); });
                    resultId = operation.messageId;
                }
                else if (operation.type == MessageOperationType::FetchGuild)
                {
                    response = PerformDiscordRequest([&]() { return httpClient_.get(L"discord.com", L"/api/v10/guilds/" + guildWide + L"?with_counts=true", headers); });
                    resultId = operation.channelId;
                    if (response.success)
                        dataStore_.storeGuild(operation.channelId, response.body);
                }
                else if (operation.type == MessageOperationType::FetchChannel)
                {
                    response = PerformDiscordRequest([&]() { return httpClient_.get(L"discord.com", L"/api/v10/channels/" + targetWide, headers); });
                    resultId = operation.messageId;
                    if (response.success)
                        dataStore_.storeChannel(operation.messageId, response.body);
                }
                else if (operation.type == MessageOperationType::FetchRole)
                {
                    response = PerformDiscordRequest([&]() { return httpClient_.get(L"discord.com", L"/api/v10/guilds/" + guildWide + L"/roles", headers); });
                    resultId = operation.messageId;
                    std::string roleJson;
                    if (response.success && DiscordDataStore::findObjectById(response.body, operation.messageId, roleJson))
                        dataStore_.storeRole(operation.channelId, operation.messageId, roleJson);
                    else if (response.success)
                        response.success = false;
                }
                else if (operation.type == MessageOperationType::FetchMember)
                {
                    response = PerformDiscordRequest([&]() { return httpClient_.get(L"discord.com", L"/api/v10/guilds/" + guildWide + L"/members/" + targetWide, headers); });
                    resultId = operation.messageId;
                    if (response.success)
                    {
                        dataStore_.storeMember(operation.channelId, operation.messageId, response.body);
                        std::string userJson, embeddedId;
                        if (DiscordDataStore::extractEmbeddedUser(response.body, userJson) && FindRootString(userJson, "id", embeddedId) && !embeddedId.empty())
                            dataStore_.storeUser(embeddedId, userJson);
                    }
                }
                else if (operation.type == MessageOperationType::FetchUser)
                {
                    response = PerformDiscordRequest([&]() { return httpClient_.get(L"discord.com", L"/api/v10/users/" + targetWide, headers); });
                    resultId = operation.messageId;
                    if (response.success)
                        dataStore_.storeUser(operation.messageId, response.body);
                }
                else if (operation.type == MessageOperationType::SetMemberNick)
                {
                    response = PerformDiscordRequest([&]() { return httpClient_.patch(L"discord.com", L"/api/v10/guilds/" + guildWide + L"/members/" + targetWide, jsonHeaders, operation.content); });
                    resultId = operation.messageId;
                    if (response.success && !response.body.empty())
                        dataStore_.storeMember(operation.channelId, operation.messageId, response.body);
                }
                else if (operation.type == MessageOperationType::SetBotProfile)
                {
                    response = PerformDiscordRequest([&]() { return httpClient_.patch(L"discord.com", L"/api/v10/users/@me", jsonHeaders, operation.content); });
                    resultId = operation.channelId;
                    std::string id;
                    if (response.success && FindRootString(response.body, "id", id) && !id.empty()) dataStore_.storeUser(id, response.body);
                }
                else if (operation.type == MessageOperationType::CreateChannel)
                {
                    response = PerformDiscordRequest([&]() { return httpClient_.post(L"discord.com", L"/api/v10/guilds/" + guildWide + L"/channels", jsonHeaders, operation.content); });
                    if (response.success)
                        FindRootString(response.body, "id", resultId);
                }
                else if (operation.type == MessageOperationType::DeleteChannel)
                {
                    response = PerformDiscordRequest([&]() { return httpClient_.del(L"discord.com", L"/api/v10/channels/" + targetWide, headers); });
                }
                else if (operation.type == MessageOperationType::CreateRole)
                {
                    response = PerformDiscordRequest([&]() { return httpClient_.post(L"discord.com", L"/api/v10/guilds/" + guildWide + L"/roles", jsonHeaders, operation.content); });
                    if (response.success)
                        FindRootString(response.body, "id", resultId);
                }
                else if (operation.type == MessageOperationType::DeleteRole)
                    response = PerformDiscordRequest([&]() { return httpClient_.del(L"discord.com", L"/api/v10/guilds/" + guildWide + L"/roles/" + targetWide, headers); });
                else if (operation.type == MessageOperationType::AddMemberRole)
                    response = PerformDiscordRequest([&]() { return httpClient_.put(L"discord.com", L"/api/v10/guilds/" + guildWide + L"/members/" + targetWide + L"/roles/" + extraWide, headers); });
                else if (operation.type == MessageOperationType::RemoveMemberRole)
                    response = PerformDiscordRequest([&]() { return httpClient_.del(L"discord.com", L"/api/v10/guilds/" + guildWide + L"/members/" + targetWide + L"/roles/" + extraWide, headers); });
                else if (operation.type == MessageOperationType::KickMember)
                    response = PerformDiscordRequest([&]() { return httpClient_.del(L"discord.com", L"/api/v10/guilds/" + guildWide + L"/members/" + targetWide, headers); });
                else if (operation.type == MessageOperationType::BanMember)
                    response = PerformDiscordRequest([&]() { return httpClient_.put(L"discord.com", L"/api/v10/guilds/" + guildWide + L"/bans/" + targetWide, jsonHeaders, operation.content); });
                else if (operation.type == MessageOperationType::UnbanMember)
                {
                    const std::wstring path = L"/api/v10/guilds/" + guildWide + L"/bans/" + targetWide;
                    response = PerformDiscordRequest([&]() { return httpClient_.del(L"discord.com", path, headers); });

                    // A API pode retornar 404 por um curto periodo logo apos um ban
                    // recem-criado. Fazemos poucas tentativas curtas apenas nesse caso.
                    for (int attempt = 0; !response.success && response.statusCode == 404 && attempt < 2; ++attempt)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(250 * (attempt + 1)));
                        response = PerformDiscordRequest([&]() { return httpClient_.del(L"discord.com", path, headers); });
                    }
                }

                success = response.success;

                if (!success)
                {
                    std::cout << "[DiscordBridge]: Guild REST falhou | Operacao: "
                              << static_cast<int>(operation.type)
                              << " | Status: " << response.statusCode
                              << " | Guild: " << operation.channelId
                              << " | Alvo: " << operation.messageId;

                    if (!response.body.empty())
                        std::cout << " | Body: " << response.body;

                    std::cout << std::endl;
                }
                {
                    std::lock_guard<std::mutex> lock(resultMutex_);
                    if (guildOperationResults_.size() >= Limits::MaxResultQueue)
                    {
                        guildOperationResults_.pop_front();
                        ++GlobalMetrics().droppedResults;
                    }
                    guildOperationResults_.push_back({operation.type, success, operation.channelId, std::move(resultId)});
                }
                continue;
            }

            case MessageOperationType::AcknowledgeInteraction:
                sendInteractionCallback(operation.channelId, operation.messageId, "{\"type\":6}");
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(resultMutex_);

                if (messageOperationResults_.size() >= Limits::MaxResultQueue)
                {
                    messageOperationResults_.pop_front();
                    ++GlobalMetrics().droppedResults;
                }
                messageOperationResults_.push_back(
                    MessageOperationResult{
                        operation.type,
                        success,
                        operation.channelId,
                        messageId});
            }
        }
    }

    bool DiscordClient::sendMessageRequest(const std::string &channelId, const std::string &message, std::string &messageId)
    {
        messageId.clear();

        if (token_.empty() || !InputValidator::snowflake(channelId) || !InputValidator::message(message))
            return false;

        const std::wstring tokenWide = Utf8ToWide(token_);
        const std::wstring channelWide = Utf8ToWide(channelId);

        if (tokenWide.empty() || channelWide.empty())
            return false;

        const std::wstring headers =
            L"Authorization: Bot " + tokenWide +
            L"\r\nContent-Type: application/json"
            L"\r\nAccept: application/json"
            L"\r\nUser-Agent: DiscordBridge/1.0.0"
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
            body);

        if (!response.success)
            return false;

        if (!FindRootString(response.body, "id", messageId))
            return false;

        return !messageId.empty();
    }

    bool DiscordClient::editMessageRequest(const std::string &channelId, const std::string &messageId, const std::string &content)
    {
        if (token_.empty() || !InputValidator::snowflake(channelId) || !InputValidator::snowflake(messageId) || !InputValidator::message(content))
            return false;

        const std::wstring tokenWide = Utf8ToWide(token_);
        const std::wstring channelWide = Utf8ToWide(channelId);
        const std::wstring messageWide = Utf8ToWide(messageId);

        if (tokenWide.empty() || channelWide.empty() || messageWide.empty())
            return false;

        const std::wstring headers =
            L"Authorization: Bot " + tokenWide +
            L"\r\nContent-Type: application/json"
            L"\r\nAccept: application/json"
            L"\r\nUser-Agent: DiscordBridge/1.0.0"
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
                              body)
            .success;
    }

    bool DiscordClient::deleteMessageRequest(const std::string &channelId, const std::string &messageId)
    {
        if (token_.empty() || !InputValidator::snowflake(channelId) || !InputValidator::snowflake(messageId))
            return false;

        const std::wstring tokenWide = Utf8ToWide(token_);
        const std::wstring channelWide = Utf8ToWide(channelId);
        const std::wstring messageWide = Utf8ToWide(messageId);

        if (tokenWide.empty() || channelWide.empty() || messageWide.empty())
            return false;

        const std::wstring headers =
            L"Authorization: Bot " + tokenWide +
            L"\r\nAccept: application/json"
            L"\r\nUser-Agent: DiscordBridge/1.0.0"
            L"\r\n";

        const std::wstring path =
            L"/api/v10/channels/" +
            channelWide +
            L"/messages/" +
            messageWide;

        return httpClient_.del(
                              L"discord.com",
                              path,
                              headers)
            .success;
    }

    bool DiscordClient::sendEmbedRequest(const std::string &channelId, const std::string &embedJson, std::string &messageId)
    {
        messageId.clear();

        if (token_.empty() || !InputValidator::snowflake(channelId) || embedJson.empty())
            return false;

        const std::wstring tokenWide = Utf8ToWide(token_);
        const std::wstring channelWide = Utf8ToWide(channelId);

        if (tokenWide.empty() || channelWide.empty())
            return false;

        const std::wstring headers =
            L"Authorization: Bot " + tokenWide +
            L"\r\nContent-Type: application/json"
            L"\r\nAccept: application/json"
            L"\r\nUser-Agent: DiscordBridge/1.0.0"
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
            body);

        if (!response.success)
            return false;

        if (!FindRootString(response.body, "id", messageId))
            return false;

        return !messageId.empty();
    }

    bool DiscordClient::sendComponentsRequest(const std::string &channelId, const std::string &componentsJson, std::string &messageId)
    {
        messageId.clear();

        if (token_.empty() || !InputValidator::snowflake(channelId) || componentsJson.empty())
            return false;

        const std::wstring tokenWide = Utf8ToWide(token_);
        const std::wstring channelWide = Utf8ToWide(channelId);

        if (tokenWide.empty() || channelWide.empty())
            return false;

        const std::wstring headers =
            L"Authorization: Bot " + tokenWide +
            L"\r\nContent-Type: application/json"
            L"\r\nAccept: application/json"
            L"\r\nUser-Agent: DiscordBridge/1.0.0"
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
            body);

        if (!response.success)
            return false;

        if (!FindRootString(response.body, "id", messageId))
            return false;

        return !messageId.empty();
    }

    bool DiscordClient::sendV2Request(const std::string &channelId, const std::string &messageJson, std::string &messageId)
    {
        messageId.clear();
        if (token_.empty() || !InputValidator::snowflake(channelId) || messageJson.empty())
            return false;
        const std::wstring tokenWide = Utf8ToWide(token_);
        const std::wstring channelWide = Utf8ToWide(channelId);
        if (tokenWide.empty() || channelWide.empty())
            return false;
        const std::wstring headers =
            L"Authorization: Bot " + tokenWide +
            L"\r\nContent-Type: application/json"
            L"\r\nAccept: application/json"
            L"\r\nUser-Agent: DiscordBridge/1.0.0"
            L"\r\n";
        const HttpResponse response = httpClient_.post(L"discord.com", L"/api/v10/channels/" + channelWide + L"/messages", headers, messageJson);
        if (!response.success)
        {
            std::cout << "[DiscordBridge]: Components V2 falhou | Status: " << response.statusCode;
            if (!response.body.empty())
                std::cout << " | Body: " << response.body;
            std::cout << std::endl;
            return false;
        }
        return FindRootString(response.body, "id", messageId) && !messageId.empty();
    }

    bool DiscordClient::editV2Request(const std::string &channelId, const std::string &messageId, const std::string &messageJson)
    {
        if (token_.empty() || !InputValidator::snowflake(channelId) || !InputValidator::snowflake(messageId) || messageJson.empty())
            return false;
        const std::wstring tokenWide = Utf8ToWide(token_);
        const std::wstring channelWide = Utf8ToWide(channelId);
        const std::wstring messageWide = Utf8ToWide(messageId);
        if (tokenWide.empty() || channelWide.empty() || messageWide.empty())
            return false;
        const std::wstring headers =
            L"Authorization: Bot " + tokenWide +
            L"\r\nContent-Type: application/json"
            L"\r\nAccept: application/json"
            L"\r\nUser-Agent: DiscordBridge/1.0.0"
            L"\r\n";
        const std::wstring path = L"/api/v10/channels/" + channelWide + L"/messages/" + messageWide;
        const HttpResponse response = httpClient_.patch(L"discord.com", path, headers, messageJson);
        if (!response.success)
        {
            std::cout << "[DiscordBridge]: Edit Components V2 falhou | Status: " << response.statusCode;
            if (!response.body.empty())
                std::cout << " | Body: " << response.body;
            std::cout << std::endl;
        }
        return response.success;
    }

    const std::string &DiscordClient::getToken() const
    {
        return token_;
    }

    const GatewayInfo &DiscordClient::getGatewayInfo() const
    {
        return gatewayInfo_;
    }
}