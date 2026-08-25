#include "Gateway.hpp"

#include <cctype>
#include <chrono>
#include <limits>
#include <utility>
#include <vector>

namespace DiscordBridge
{
    namespace
    {
        constexpr wchar_t GATEWAY_HOST[] = L"gateway.discord.gg";
        constexpr wchar_t GATEWAY_PATH[] = L"/?v=10&encoding=json";
        constexpr wchar_t USER_AGENT[] = L"DiscordBridge/0.0.4";

        constexpr std::uint32_t GATEWAY_INTENTS = (1u << 0) | (1u << 1) | (1u << 9) | (1u << 15);

        bool FindInteger(const std::string& json, const std::string& key, std::int64_t& value, std::size_t startPosition = 0)
        {
            const std::string search = "\"" + key + "\"";
            std::size_t position = json.find(search, startPosition);
            if (position == std::string::npos) return false;

            position = json.find(':', position + search.size());
            if (position == std::string::npos) return false;

            ++position;

            while (position < json.size() && std::isspace(static_cast<unsigned char>(json[position]))) ++position;
            if (position >= json.size()) return false;

            bool negative = false;

            if (json[position] == '-')
            {
                negative = true;
                ++position;
            }

            if (position >= json.size() || !std::isdigit(static_cast<unsigned char>(json[position]))) return false;

            std::int64_t result = 0;

            while (position < json.size() && std::isdigit(static_cast<unsigned char>(json[position])))
            {
                const int digit = json[position] - '0';

                if (result > (std::numeric_limits<std::int64_t>::max() - digit) / 10) return false;

                result = (result * 10) + digit;
                ++position;
            }

            value = negative ? -result : result;
            return true;
        }

        bool FindInteger(const std::string& json, const std::string& key, int& value, std::size_t startPosition = 0)
        {
            std::int64_t parsed = 0;

            if (!FindInteger(json, key, parsed, startPosition)) return false;
            if (parsed < std::numeric_limits<int>::min() || parsed > std::numeric_limits<int>::max()) return false;

            value = static_cast<int>(parsed);
            return true;
        }

        bool FindString(const std::string& json, const std::string& key, std::string& value, std::size_t startPosition = 0)
        {
            const std::string search = "\"" + key + "\"";
            std::size_t position = json.find(search, startPosition);
            if (position == std::string::npos) return false;

            position = json.find(':', position + search.size());
            if (position == std::string::npos) return false;

            position = json.find('"', position + 1);
            if (position == std::string::npos) return false;

            ++position;

            std::string result;
            result.reserve(64);

            bool escaped = false;

            while (position < json.size())
            {
                const char character = json[position++];

                if (escaped)
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
                        default:
                            result.push_back('\\');
                            result.push_back(character);
                            break;
                    }

                    escaped = false;
                    continue;
                }

                if (character == '\\')
                {
                    escaped = true;
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

        bool FindObjectString(const std::string& json, const std::string& objectKey, const std::string& key, std::string& value)
        {
            const std::string search = "\"" + objectKey + "\"";
            std::size_t position = json.find(search);
            if (position == std::string::npos) return false;

            position = json.find(':', position + search.size());
            if (position == std::string::npos) return false;

            const std::size_t objectStart = json.find('{', position + 1);
            if (objectStart == std::string::npos) return false;

            position = objectStart + 1;

            int depth = 1;
            bool insideString = false;
            bool escaped = false;

            while (position < json.size())
            {
                const char character = json[position];

                if (insideString)
                {
                    if (escaped) escaped = false;
                    else if (character == '\\') escaped = true;
                    else if (character == '"') insideString = false;
                }
                else
                {
                    if (character == '"') insideString = true;
                    else if (character == '{') ++depth;
                    else if (character == '}' && --depth == 0) return FindString(json.substr(objectStart, position - objectStart + 1), key, value);
                }

                ++position;
            }

            return false;
        }

        bool FindNestedObjectString(const std::string& json, const std::string& parentKey, const std::string& objectKey, const std::string& key, std::string& value)
        {
            const std::string search = "\"" + parentKey + "\"";
            std::size_t position = json.find(search);
            if (position == std::string::npos) return false;

            position = json.find(':', position + search.size());
            if (position == std::string::npos) return false;

            const std::size_t objectStart = json.find('{', position + 1);
            if (objectStart == std::string::npos) return false;

            position = objectStart + 1;

            int depth = 1;
            bool insideString = false;
            bool escaped = false;

            while (position < json.size())
            {
                const char character = json[position];

                if (insideString)
                {
                    if (escaped) escaped = false;
                    else if (character == '\\') escaped = true;
                    else if (character == '"') insideString = false;
                }
                else
                {
                    if (character == '"') insideString = true;
                    else if (character == '{') ++depth;
                    else if (character == '}' && --depth == 0)
                    {
                        const std::string parentJson = json.substr(objectStart, position - objectStart + 1);
                        return FindObjectString(parentJson, objectKey, key, value);
                    }
                }

                ++position;
            }

            return false;
        }


        bool ParseJsonStringAt(const std::string& json, std::size_t position, std::string& value, std::size_t& endPosition)
        {
            if (position >= json.size() || json[position] != '"') return false;
            ++position;
            std::string result;
            bool escaped = false;
            while (position < json.size())
            {
                const char character = json[position++];
                if (escaped)
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
                    escaped = false;
                    continue;
                }
                if (character == '\\') { escaped = true; continue; }
                if (character == '"') { value = std::move(result); endPosition = position; return true; }
                result.push_back(character);
            }
            return false;
        }

        bool FindDirectValue(const std::string& json, const std::string& key, std::size_t& valuePosition)
        {
            int objectDepth = 0;
            int arrayDepth = 0;
            std::size_t position = 0;
            while (position < json.size())
            {
                const char character = json[position];
                if (character == '{') { ++objectDepth; ++position; continue; }
                if (character == '}') { --objectDepth; ++position; continue; }
                if (character == '[') { ++arrayDepth; ++position; continue; }
                if (character == ']') { --arrayDepth; ++position; continue; }
                if (character != '"') { ++position; continue; }

                std::string parsedKey;
                std::size_t afterString = position;
                if (!ParseJsonStringAt(json, position, parsedKey, afterString)) return false;

                if (objectDepth == 1 && arrayDepth == 0)
                {
                    std::size_t colon = afterString;
                    while (colon < json.size() && std::isspace(static_cast<unsigned char>(json[colon]))) ++colon;
                    if (colon < json.size() && json[colon] == ':')
                    {
                        if (parsedKey == key)
                        {
                            valuePosition = colon + 1;
                            while (valuePosition < json.size() && std::isspace(static_cast<unsigned char>(json[valuePosition]))) ++valuePosition;
                            return valuePosition < json.size();
                        }
                    }
                }

                position = afterString;
            }
            return false;
        }

        bool FindDirectString(const std::string& json, const std::string& key, std::string& value)
        {
            std::size_t position = 0;
            if (!FindDirectValue(json, key, position)) return false;
            std::size_t endPosition = position;
            return ParseJsonStringAt(json, position, value, endPosition);
        }

        bool FindDirectInteger(const std::string& json, const std::string& key, std::int64_t& value)
        {
            std::size_t position = 0;
            if (!FindDirectValue(json, key, position)) return false;
            bool negative = false;
            if (json[position] == '-') { negative = true; ++position; }
            if (position >= json.size() || !std::isdigit(static_cast<unsigned char>(json[position]))) return false;
            std::int64_t result = 0;
            while (position < json.size() && std::isdigit(static_cast<unsigned char>(json[position])))
            {
                const int digit = json[position] - '0';
                if (result > (std::numeric_limits<std::int64_t>::max() - digit) / 10) return false;
                result = result * 10 + digit;
                ++position;
            }
            value = negative ? -result : result;
            return true;
        }

        bool FindDirectObject(const std::string& json, const std::string& key, std::string& value)
        {
            std::size_t position = 0;
            if (!FindDirectValue(json, key, position) || json[position] != '{') return false;
            const std::size_t start = position;
            int depth = 0;
            bool insideString = false;
            bool escaped = false;
            while (position < json.size())
            {
                const char character = json[position++];
                if (insideString)
                {
                    if (escaped) escaped = false;
                    else if (character == '\\') escaped = true;
                    else if (character == '"') insideString = false;
                    continue;
                }
                if (character == '"') { insideString = true; continue; }
                if (character == '{') ++depth;
                else if (character == '}' && --depth == 0) { value = json.substr(start, position - start); return true; }
            }
            return false;
        }

        bool FindDirectStringArrayFirst(const std::string& json, const std::string& key, std::string& value)
        {
            std::size_t position = 0;
            if (!FindDirectValue(json, key, position) || json[position] != '[') return false;
            ++position;
            while (position < json.size() && std::isspace(static_cast<unsigned char>(json[position]))) ++position;
            if (position >= json.size() || json[position] != '"') return false;
            std::size_t endPosition = position;
            return ParseJsonStringAt(json, position, value, endPosition);
        }


        void ExtractModalValues(const std::string& json, std::vector<std::pair<std::string, std::string>>& values)
        {
            values.clear();
            std::vector<std::pair<std::string, std::string>> parsed;
            std::vector<std::size_t> objectStarts;
            bool insideString = false;
            bool escaped = false;

            for (std::size_t position = 0; position < json.size(); ++position)
            {
                const char character = json[position];
                if (insideString)
                {
                    if (escaped) escaped = false;
                    else if (character == '\\') escaped = true;
                    else if (character == '"') insideString = false;
                    continue;
                }
                if (character == '"') { insideString = true; continue; }
                if (character == '{') { objectStarts.push_back(position); continue; }
                if (character != '}' || objectStarts.empty()) continue;

                const std::size_t start = objectStarts.back();
                objectStarts.pop_back();
                const std::string objectJson = json.substr(start, position - start + 1);
                std::string customId;
                std::string value;
                if (!FindDirectString(objectJson, "custom_id", customId) || !FindDirectString(objectJson, "value", value) || customId.empty()) continue;

                bool replaced = false;
                for (auto& entry : parsed)
                {
                    if (entry.first != customId) continue;
                    entry.second = std::move(value);
                    replaced = true;
                    break;
                }
                if (!replaced) parsed.emplace_back(std::move(customId), std::move(value));
            }
            values = std::move(parsed);
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

        const char* GetStatusName(int status)
        {
            switch (status)
            {
                case 0: return "online";
                case 1: return "idle";
                case 2: return "dnd";
                case 3: return "invisible";
                case 4: return "offline";
                default: return nullptr;
            }
        }
    }

    bool GatewayInfo::isValid() const
    {
        return !url.empty() && shards > 0;
    }

    bool ParseGatewayInfo(const std::string& json, GatewayInfo& info)
    {
        if (json.empty()) return false;

        GatewayInfo parsed;

        if (!FindString(json, "url", parsed.url)) return false;

        FindInteger(json, "shards", parsed.shards);

        const std::size_t sessionPosition = json.find("\"session_start_limit\"");

        if (sessionPosition != std::string::npos)
        {
            FindInteger(json, "total", parsed.sessionTotal, sessionPosition);
            FindInteger(json, "remaining", parsed.sessionRemaining, sessionPosition);
            FindInteger(json, "reset_after", parsed.sessionResetAfter, sessionPosition);
            FindInteger(json, "max_concurrency", parsed.maxConcurrency, sessionPosition);
        }

        if (!parsed.isValid()) return false;

        info = std::move(parsed);
        return true;
    }

    Gateway::~Gateway()
    {
        disconnect();
    }

    bool Gateway::connect(const GatewayInfo& gatewayInfo, const std::string& token)
    {
        if (initialized_ || !gatewayInfo.isValid() || token.empty()) return false;

        token_ = token;
        resetState();

        session_ = WinHttpOpen(USER_AGENT, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!session_) return failConnection();

        connection_ = WinHttpConnect(session_, GATEWAY_HOST, INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!connection_) return failConnection();

        request_ = WinHttpOpenRequest(connection_, L"GET", GATEWAY_PATH, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (!request_) return failConnection();

        if (!WinHttpSetOption(request_, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, nullptr, 0)) return failConnection();
        if (!WinHttpSendRequest(request_, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) return failConnection();
        if (!WinHttpReceiveResponse(request_, nullptr)) return failConnection();

        DWORD statusCode = 0;
        DWORD statusCodeSize = sizeof(statusCode);

        if (!WinHttpQueryHeaders(request_, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX) || statusCode != 101) return failConnection();

        webSocket_ = WinHttpWebSocketCompleteUpgrade(request_, 0);
        if (!webSocket_) return failConnection();

        WinHttpCloseHandle(request_);
        request_ = nullptr;

        initialized_ = true;
        connected_ = true;
        running_ = true;

        try
        {
            receiveThread_ = std::thread(&Gateway::receiveLoop, this);
        }
        catch (...)
        {
            return failConnection();
        }

        return true;
    }

    void Gateway::disconnect()
    {
        running_ = false;
        ready_ = false;
        connected_ = false;
        initialized_ = false;
        readyEventPending_ = false;

        heartbeatCondition_.notify_all();

        if (webSocket_)
        {
            HINTERNET socket = webSocket_;
            webSocket_ = nullptr;

            WinHttpWebSocketShutdown(socket, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, nullptr, 0);
            WinHttpCloseHandle(socket);
        }

        if (heartbeatThread_.joinable() && heartbeatThread_.get_id() != std::this_thread::get_id()) heartbeatThread_.join();
        if (receiveThread_.joinable() && receiveThread_.get_id() != std::this_thread::get_id()) receiveThread_.join();

        closeHandles();
        resetState();

        token_.clear();

        {
            std::lock_guard<std::mutex> lock(presenceMutex_);
            currentStatus_ = 0;
            currentActivityType_ = 0;
            currentActivityName_.clear();
            currentActivityState_.clear();
            currentActivityUrl_.clear();
            currentAfk_ = false;
        }
    }

    bool Gateway::isInitialized() const
    {
        return initialized_;
    }

    bool Gateway::isConnected() const
    {
        return connected_;
    }

    bool Gateway::isReady() const
    {
        return ready_;
    }

    bool Gateway::consumeReadyEvent()
    {
        return readyEventPending_.exchange(false);
    }

    bool Gateway::consumeMessageCreateEvent(std::string& userId, std::string& channelId, std::string& message)
    {
        std::lock_guard<std::mutex> lock(eventMutex_);

        if (messageEvents_.empty()) return false;

        MessageCreateEvent event = std::move(messageEvents_.front());
        messageEvents_.pop_front();

        userId = std::move(event.userId);
        channelId = std::move(event.channelId);
        message = std::move(event.message);

        return true;
    }

    bool Gateway::consumeGuildMemberAddEvent(std::string& guildId, std::string& userId)
    {
        return consumeGuildMemberEvent(guildMemberAddEvents_, guildId, userId);
    }

    bool Gateway::consumeGuildMemberRemoveEvent(std::string& guildId, std::string& userId)
    {
        return consumeGuildMemberEvent(guildMemberRemoveEvents_, guildId, userId);
    }

    bool Gateway::consumeButtonClickEvent(std::string& interactionId, std::string& interactionToken, std::string& userId, std::string& channelId, std::string& customId)
    {
        std::lock_guard<std::mutex> lock(eventMutex_);

        if (buttonClickEvents_.empty()) return false;

        ComponentEvent event = std::move(buttonClickEvents_.front());
        buttonClickEvents_.pop_front();

        interactionId = std::move(event.interactionId);
        interactionToken = std::move(event.interactionToken);
        userId = std::move(event.userId);
        channelId = std::move(event.channelId);
        customId = std::move(event.customId);

        return true;
    }

    bool Gateway::consumeSelectMenuEvent(std::string& interactionId, std::string& interactionToken, std::string& userId, std::string& channelId, std::string& customId, std::string& value)
    {
        std::lock_guard<std::mutex> lock(eventMutex_); if(selectMenuEvents_.empty()) return false; ComponentEvent event=std::move(selectMenuEvents_.front()); selectMenuEvents_.pop_front(); interactionId=std::move(event.interactionId); interactionToken=std::move(event.interactionToken); userId=std::move(event.userId); channelId=std::move(event.channelId); customId=std::move(event.customId); value=std::move(event.value); return true;
    }

    bool Gateway::consumeModalEvent(std::string& interactionId, std::string& interactionToken, std::string& userId, std::string& channelId, std::string& customId, std::vector<std::pair<std::string, std::string>>& values)
    {
        std::lock_guard<std::mutex> lock(eventMutex_);
        if (modalEvents_.empty()) return false;
        ComponentEvent event = std::move(modalEvents_.front());
        modalEvents_.pop_front();
        interactionId = std::move(event.interactionId);
        interactionToken = std::move(event.interactionToken);
        userId = std::move(event.userId);
        channelId = std::move(event.channelId);
        customId = std::move(event.customId);
        values = std::move(event.modalValues);
        return true;
    }

    bool Gateway::setStatus(int status)
    {
        if (!GetStatusName(status)) return false;

        {
            std::lock_guard<std::mutex> lock(presenceMutex_);
            currentStatus_ = status;
        }

        return !ready_ || sendPresence();
    }

    bool Gateway::setActivity(int type, const std::string& name, const std::string& state, const std::string& url)
    {
        if (type < 0 || type > 5 || name.empty()) return false;

        {
            std::lock_guard<std::mutex> lock(presenceMutex_);
            currentActivityType_ = type;
            currentActivityName_ = name;
            currentActivityState_ = state;
            currentActivityUrl_ = url;
        }

        return !ready_ || sendPresence();
    }

    bool Gateway::clearActivity()
    {
        {
            std::lock_guard<std::mutex> lock(presenceMutex_);
            currentActivityType_ = 0;
            currentActivityName_.clear();
            currentActivityState_.clear();
            currentActivityUrl_.clear();
        }

        return !ready_ || sendPresence();
    }

    bool Gateway::setPresence(int status, int activityType, const std::string& name, const std::string& state, const std::string& url, bool afk)
    {
        if (!GetStatusName(status) || activityType < 0 || activityType > 5) return false;

        {
            std::lock_guard<std::mutex> lock(presenceMutex_);
            currentStatus_ = status;
            currentActivityType_ = activityType;
            currentActivityName_ = name;
            currentActivityState_ = state;
            currentActivityUrl_ = url;
            currentAfk_ = afk;
        }

        return !ready_ || sendPresence();
    }

    void Gateway::receiveLoop()
    {
        std::string payload;

        while (running_)
        {
            if (!receivePayload(payload) || !running_) break;

            handlePayload(payload);
        }

        stopConnection();
    }

    void Gateway::heartbeatLoop()
    {
        std::unique_lock<std::mutex> lock(heartbeatMutex_);

        while (running_)
        {
            const std::uint32_t interval = heartbeatInterval_.load();

            if (interval == 0)
            {
                heartbeatCondition_.wait(lock, [this]() { return !running_.load() || heartbeatInterval_.load() > 0; });
                continue;
            }

            if (heartbeatCondition_.wait_for(lock, std::chrono::milliseconds(interval), [this]() { return !running_.load(); }) || !running_) break;

            if (!heartbeatAck_)
            {
                stopConnection();
                break;
            }

            heartbeatAck_ = false;

            lock.unlock();
            const bool sent = sendHeartbeat();
            lock.lock();

            if (!sent)
            {
                stopConnection();
                break;
            }
        }
    }

    bool Gateway::receivePayload(std::string& payload)
    {
        payload.clear();

        HINTERNET socket = webSocket_;

        if (!running_ || !socket) return false;

        std::vector<char> buffer(8192);

        while (running_)
        {
            DWORD bytesRead = 0;
            WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType{};

            const DWORD result = WinHttpWebSocketReceive(socket, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, &bufferType);

            if (result != NO_ERROR || !running_ || bufferType == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) return false;

            if (bytesRead > 0) payload.append(buffer.data(), bytesRead);

            if (bufferType == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE) return true;
            if (bufferType != WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE) return false;
        }

        return false;
    }

    bool Gateway::sendText(const std::string& payload)
    {
        if (payload.empty()) return false;

        std::lock_guard<std::mutex> lock(sendMutex_);

        HINTERNET socket = webSocket_;
        if (!running_ || !socket) return false;

        return WinHttpWebSocketSend(
            socket,
            WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
            const_cast<char*>(payload.data()),
            static_cast<DWORD>(payload.size())
        ) == NO_ERROR;
    }

    bool Gateway::sendHeartbeat()
    {
        if (!running_) return false;

        const std::int64_t sequence = sequence_.load();

        std::string payload = "{\"op\":1,\"d\":";

        if (sequence < 0) payload += "null";
        else payload += std::to_string(sequence);

        payload += "}";

        return sendText(payload);
    }

    bool Gateway::sendIdentify()
    {
        if (!running_ || token_.empty()) return false;

        const std::string payload = "{\"op\":2,\"d\":{\"token\":\"" + EscapeJson(token_) + "\",\"intents\":" + std::to_string(GATEWAY_INTENTS) + ",\"properties\":{\"os\":\"windows\",\"browser\":\"discordbridge\",\"device\":\"discordbridge\"}}}";

        return sendText(payload);
    }

    bool Gateway::sendPresence()
    {
        if (!running_ || !ready_) return false;

        int statusValue = 0;
        int activityType = 0;
        bool afk = false;
        std::string activityName;
        std::string activityState;
        std::string activityUrl;

        {
            std::lock_guard<std::mutex> lock(presenceMutex_);
            statusValue = currentStatus_;
            activityType = currentActivityType_;
            afk = currentAfk_;
            activityName = currentActivityName_;
            activityState = currentActivityState_;
            activityUrl = currentActivityUrl_;
        }

        const char* status = GetStatusName(statusValue);
        if (!status) return false;

        std::string activities = "[]";

        if (!activityName.empty())
        {
            activities = "[{\"name\":\"" + EscapeJson(activityName) + "\",\"type\":" + std::to_string(activityType);

            if (!activityState.empty()) activities += ",\"state\":\"" + EscapeJson(activityState) + "\"";
            if (!activityUrl.empty()) activities += ",\"url\":\"" + EscapeJson(activityUrl) + "\"";

            activities += "}]";
        }

        const std::string payload = "{\"op\":3,\"d\":{\"since\":null,\"activities\":" + activities + ",\"status\":\"" + std::string(status) + "\",\"afk\":" + std::string(afk ? "true" : "false") + "}}";

        return sendText(payload);
    }

    void Gateway::handlePayload(const std::string& payload)
    {
        if (!running_) return;

        std::int64_t sequence = 0;

        if (FindInteger(payload, "s", sequence)) sequence_ = sequence;

        std::int64_t opcode = -1;

        if (!FindInteger(payload, "op", opcode)) return;

        switch (opcode)
        {
            case 0:
                handleDispatch(payload);
                break;

            case 1:
                if (!sendHeartbeat()) stopConnection();
                break;

            case 10:
                handleHello(payload);
                break;

            case 11:
                heartbeatAck_ = true;
                break;
        }
    }

    void Gateway::handleHello(const std::string& payload)
    {
        std::int64_t interval = 0;

        if (!FindInteger(payload, "heartbeat_interval", interval) || interval <= 0 || interval > std::numeric_limits<std::uint32_t>::max()) return;

        heartbeatInterval_ = static_cast<std::uint32_t>(interval);
        heartbeatAck_ = true;

        if (!heartbeatThread_.joinable())
        {
            try
            {
                heartbeatThread_ = std::thread(&Gateway::heartbeatLoop, this);
            }
            catch (...)
            {
                stopConnection();
                return;
            }
        }

        heartbeatCondition_.notify_all();

        if (!sendIdentify()) stopConnection();
    }

    void Gateway::handleDispatch(const std::string& payload)
    {
        std::string eventName;

        if (!FindString(payload, "t", eventName)) return;

        if (eventName == "READY")
        {
            ready_ = true;
            connected_ = true;
            readyEventPending_ = true;

            sendPresence();

            return;
        }

        if (eventName == "INTERACTION_CREATE")
        {
            std::string interactionJson;
            if (!FindDirectObject(payload, "d", interactionJson)) return;

            std::int64_t interactionType = 0;
            if (!FindDirectInteger(interactionJson, "type", interactionType)) return;

            ComponentEvent event;
            if (!FindDirectString(interactionJson, "id", event.interactionId) || !FindDirectString(interactionJson, "token", event.interactionToken) || !FindDirectString(interactionJson, "channel_id", event.channelId)) return;

            std::string memberJson;
            std::string userJson;
            if (FindDirectObject(interactionJson, "member", memberJson) && FindDirectObject(memberJson, "user", userJson))
            {
                if (!FindDirectString(userJson, "id", event.userId)) return;
            }
            else if (FindDirectObject(interactionJson, "user", userJson))
            {
                if (!FindDirectString(userJson, "id", event.userId)) return;
            }
            else return;

            std::string dataJson;
            if (!FindDirectObject(interactionJson, "data", dataJson)) return;
            if (!FindDirectString(dataJson, "custom_id", event.customId)) return;

            std::lock_guard<std::mutex> lock(eventMutex_);

            if (interactionType == 3)
            {
                std::int64_t componentType = 0;
                if (!FindDirectInteger(dataJson, "component_type", componentType)) return;

                if (componentType == 2) buttonClickEvents_.push_back(std::move(event));
                else if (componentType == 3)
                {
                    FindDirectStringArrayFirst(dataJson, "values", event.value);
                    selectMenuEvents_.push_back(std::move(event));
                }
            }
            else if (interactionType == 5)
            {
                ExtractModalValues(dataJson, event.modalValues);
                modalEvents_.push_back(std::move(event));
            }

            return;
        }

        if (eventName == "MESSAGE_CREATE")
        {
            MessageCreateEvent event;

            if (!FindObjectString(payload, "author", "id", event.userId)) return;
            if (!FindString(payload, "channel_id", event.channelId)) return;
            if (!FindString(payload, "content", event.message)) return;

            std::lock_guard<std::mutex> lock(eventMutex_);
            messageEvents_.push_back(std::move(event));

            return;
        }

        if (eventName == "GUILD_MEMBER_ADD")
        {
            GuildMemberEvent event;

            if (!FindString(payload, "guild_id", event.guildId)) return;
            if (!FindObjectString(payload, "user", "id", event.userId)) return;

            std::lock_guard<std::mutex> lock(eventMutex_);
            guildMemberAddEvents_.push_back(std::move(event));

            return;
        }

        if (eventName == "GUILD_MEMBER_REMOVE")
        {
            GuildMemberEvent event;

            if (!FindString(payload, "guild_id", event.guildId)) return;
            if (!FindObjectString(payload, "user", "id", event.userId)) return;

            std::lock_guard<std::mutex> lock(eventMutex_);
            guildMemberRemoveEvents_.push_back(std::move(event));
        }
    }

    bool Gateway::consumeGuildMemberEvent(std::deque<GuildMemberEvent>& events, std::string& guildId, std::string& userId)
    {
        std::lock_guard<std::mutex> lock(eventMutex_);

        if (events.empty()) return false;

        GuildMemberEvent event = std::move(events.front());
        events.pop_front();

        guildId = std::move(event.guildId);
        userId = std::move(event.userId);

        return true;
    }

    bool Gateway::failConnection()
    {
        running_ = false;
        ready_ = false;
        connected_ = false;
        initialized_ = false;

        closeHandles();
        token_.clear();

        return false;
    }

    void Gateway::stopConnection()
    {
        running_ = false;
        connected_ = false;
        ready_ = false;

        heartbeatCondition_.notify_all();
    }

    void Gateway::closeHandles()
    {
        if (webSocket_)
        {
            WinHttpCloseHandle(webSocket_);
            webSocket_ = nullptr;
        }

        if (request_)
        {
            WinHttpCloseHandle(request_);
            request_ = nullptr;
        }

        if (connection_)
        {
            WinHttpCloseHandle(connection_);
            connection_ = nullptr;
        }

        if (session_)
        {
            WinHttpCloseHandle(session_);
            session_ = nullptr;
        }
    }

    void Gateway::resetState()
    {
        ready_ = false;
        readyEventPending_ = false;
        heartbeatAck_ = true;
        heartbeatInterval_ = 0;
        sequence_ = -1;

        std::lock_guard<std::mutex> lock(eventMutex_);

        messageEvents_.clear();
        guildMemberAddEvents_.clear();
        guildMemberRemoveEvents_.clear();
        buttonClickEvents_.clear();
        selectMenuEvents_.clear();
        modalEvents_.clear();
    }
}