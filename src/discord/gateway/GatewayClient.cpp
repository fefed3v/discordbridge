#include "GatewayClient.hpp"

#include <chrono>
#include <cctype>
#include <string>
#include <thread>
#include <vector>

namespace DiscordBridge
{
    static bool FindInteger(const std::string& json, const std::string& key, std::int64_t& value)
    {
        const std::string search = "\"" + key + "\"";

        std::size_t position = json.find(search);
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
            result = (result * 10) + static_cast<std::int64_t>(json[position] - '0');
            ++position;
        }

        value = negative ? -result : result;
        return true;
    }

    static bool FindString(const std::string& json, const std::string& key, std::string& value)
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

    GatewayClient::~GatewayClient()
    {
        disconnect();
    }

    bool GatewayClient::connect(const GatewayInfo& gatewayInfo, const std::string& token)
    {
        if (initialized_) return false;
        if (!gatewayInfo.isValid()) return false;
        if (token.empty()) return false;

        token_ = token;

        initialized_ = true;
        connected_ = true;
        running_ = true;
        ready_ = false;
        readyEventPending_ = false;
        heartbeatAck_ = true;
        heartbeatInterval_ = 0;
        sequence_ = -1;

        const std::wstring host = L"gateway.discord.gg";
        const std::wstring path = L"/?v=10&encoding=json";

        session_ = WinHttpOpen(
            L"DiscordBridge/0.0.1",
            WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
        );

        if (session_ == nullptr)
        {
            closeHandles();
            token_.clear();
            return false;
        }

        connection_ = WinHttpConnect(
            session_,
            host.c_str(),
            INTERNET_DEFAULT_HTTPS_PORT,
            0
        );

        if (connection_ == nullptr)
        {
            closeHandles();
            token_.clear();
            return false;
        }

        request_ = WinHttpOpenRequest(
            connection_,
            L"GET",
            path.c_str(),
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE
        );

        if (request_ == nullptr)
        {
            closeHandles();
            token_.clear();
            return false;
        }

        if (!WinHttpSetOption(
            request_,
            WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET,
            nullptr,
            0
        ))
        {
            closeHandles();
            token_.clear();
            return false;
        }

        if (!WinHttpSendRequest(
            request_,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            WINHTTP_NO_REQUEST_DATA,
            0,
            0,
            0
        ))
        {
            closeHandles();
            token_.clear();
            return false;
        }

        if (!WinHttpReceiveResponse(request_, nullptr))
        {
            closeHandles();
            token_.clear();
            return false;
        }

        DWORD statusCode = 0;
        DWORD statusCodeSize = sizeof(statusCode);

        if (!WinHttpQueryHeaders(
            request_,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &statusCode,
            &statusCodeSize,
            WINHTTP_NO_HEADER_INDEX
        ))
        {
            closeHandles();
            token_.clear();
            return false;
        }

        if (statusCode != 101)
        {
            closeHandles();
            token_.clear();
            return false;
        }

        webSocket_ = WinHttpWebSocketCompleteUpgrade(request_, 0);

        if (webSocket_ == nullptr)
        {
            closeHandles();
            token_.clear();
            return false;
        }

        WinHttpCloseHandle(request_);
        request_ = nullptr;

        initialized_ = true;
        connected_ = true;
        running_ = true;
        ready_ = false;
        heartbeatAck_ = true;
        heartbeatInterval_ = 0;
        sequence_ = -1;

        try
        {
            receiveThread_ = std::thread(&GatewayClient::receiveLoop, this);
        }
        catch (...)
        {
            running_ = false;
            connected_ = false;
            initialized_ = false;

            closeHandles();
            token_.clear();

            return false;
        }

        return true;
    }

    void GatewayClient::disconnect()
    {
        running_ = false;
        ready_ = false;
        readyEventPending_ = false;
        connected_ = false;
        initialized_ = false;

        heartbeatCondition_.notify_all();

        HINTERNET socket = webSocket_;
        webSocket_ = nullptr;

        if (socket != nullptr)
        {
            WinHttpWebSocketShutdown(
                socket,
                WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS,
                nullptr,
                0
            );

            WinHttpCloseHandle(socket);
        }

        if (heartbeatThread_.joinable() && heartbeatThread_.get_id() != std::this_thread::get_id())
        {
            heartbeatThread_.join();
        }

        if (receiveThread_.joinable() && receiveThread_.get_id() != std::this_thread::get_id())
        {
            receiveThread_.join();
        }

        closeHandles();

        token_.clear();

        heartbeatInterval_ = 0;
        heartbeatAck_ = true;
        sequence_ = -1;
    }

    bool GatewayClient::isInitialized() const
    {
        return initialized_;
    }

    bool GatewayClient::isConnected() const
    {
        return connected_;
    }

    bool GatewayClient::isReady() const
    {
        return ready_;
    }

    bool GatewayClient::consumeReadyEvent()
    {
        return readyEventPending_.exchange(false);
    }

    void GatewayClient::receiveLoop()
    {
        std::string payload;

        while (running_)
        {
            if (!receivePayload(payload))
            {
                break;
            }

            if (!running_) break;

            handlePayload(payload);
        }

        running_ = false;
        connected_ = false;
        ready_ = false;

        heartbeatCondition_.notify_all();
    }

    void GatewayClient::heartbeatLoop()
    {
        std::unique_lock<std::mutex> lock(heartbeatMutex_);

        while (running_)
        {
            const std::uint32_t interval = heartbeatInterval_.load();

            if (interval == 0)
            {
                heartbeatCondition_.wait(
                    lock,
                    [this]()
                    {
                        return !running_.load() || heartbeatInterval_.load() > 0;
                    }
                );

                continue;
            }

            const bool interrupted = heartbeatCondition_.wait_for(
                lock,
                std::chrono::milliseconds(interval),
                [this]()
                {
                    return !running_.load();
                }
            );

            if (interrupted || !running_) break;

            if (!heartbeatAck_)
            {
                running_ = false;
                connected_ = false;
                ready_ = false;

                heartbeatCondition_.notify_all();
                break;
            }

            heartbeatAck_ = false;

            lock.unlock();

            const bool sent = sendHeartbeat();

            lock.lock();

            if (!sent)
            {
                running_ = false;
                connected_ = false;
                ready_ = false;

                heartbeatCondition_.notify_all();
                break;
            }
        }
    }

    bool GatewayClient::receivePayload(std::string& payload)
    {
        payload.clear();

        if (!running_) return false;

        HINTERNET socket = webSocket_;

        if (socket == nullptr) return false;

        std::vector<char> buffer(8192);

        while (running_)
        {
            DWORD bytesRead = 0;
            WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType{};

            const DWORD result = WinHttpWebSocketReceive(
                socket,
                buffer.data(),
                static_cast<DWORD>(buffer.size()),
                &bytesRead,
                &bufferType
            );

            if (result != NO_ERROR) return false;
            if (!running_) return false;
            if (bufferType == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) return false;

            if (bytesRead > 0)
            {
                payload.append(buffer.data(), bytesRead);
            }

            if (bufferType == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE)
            {
                return true;
            }

            if (bufferType == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE)
            {
                continue;
            }

            return false;
        }

        return false;
    }

    bool GatewayClient::sendText(const std::string& payload)
    {
        if (!running_) return false;
        if (payload.empty()) return false;

        HINTERNET socket = webSocket_;

        if (socket == nullptr) return false;

        const DWORD result = WinHttpWebSocketSend(
            socket,
            WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
            const_cast<char*>(payload.data()),
            static_cast<DWORD>(payload.size())
        );

        return result == NO_ERROR;
    }

    bool GatewayClient::sendHeartbeat()
    {
        if (!running_) return false;

        const std::int64_t sequence = sequence_.load();

        std::string payload = "{\"op\":1,\"d\":";

        if (sequence < 0)
        {
            payload += "null";
        }
        else
        {
            payload += std::to_string(sequence);
        }

        payload += "}";

        return sendText(payload);
    }

    bool GatewayClient::sendIdentify()
    {
        if (!running_) return false;
        if (token_.empty()) return false;

        constexpr std::uint32_t intents =
            (1u << 0) |
            (1u << 1) |
            (1u << 9) |
            (1u << 15);

        const std::string payload =
            "{\"op\":2,\"d\":{"
            "\"token\":\"" + token_ + "\","
            "\"intents\":" + std::to_string(intents) + ","
            "\"properties\":{"
            "\"os\":\"windows\","
            "\"browser\":\"discordbridge\","
            "\"device\":\"discordbridge\""
            "}}}";

        return sendText(payload);
    }

    void GatewayClient::handlePayload(const std::string& payload)
    {
        if (!running_) return;

        std::int64_t sequence = 0;

        if (FindInteger(payload, "s", sequence))
        {
            sequence_ = sequence;
        }

        std::int64_t opcode = -1;

        if (!FindInteger(payload, "op", opcode))
        {
            return;
        }

        if (opcode == 10)
        {
            std::int64_t interval = 0;

            if (!FindInteger(payload, "heartbeat_interval", interval)) return;
            if (interval <= 0) return;

            heartbeatInterval_ = static_cast<std::uint32_t>(interval);
            heartbeatAck_ = true;

            if (!heartbeatThread_.joinable())
            {
                try
                {
                    heartbeatThread_ = std::thread(
                        &GatewayClient::heartbeatLoop,
                        this
                    );
                }
                catch (...)
                {
                    running_ = false;
                    connected_ = false;
                    ready_ = false;
                    return;
                }
            }

            heartbeatCondition_.notify_all();

            if (!sendIdentify())
            {
                running_ = false;
                connected_ = false;
                ready_ = false;

                heartbeatCondition_.notify_all();
            }

            return;
        }

        if (opcode == 1)
        {
            if (!sendHeartbeat())
            {
                running_ = false;
                connected_ = false;
                ready_ = false;

                heartbeatCondition_.notify_all();
            }

            return;
        }

        if (opcode == 11)
        {
            heartbeatAck_ = true;
            return;
        }

        if (opcode != 0) return;

        std::string eventName;

        if (!FindString(payload, "t", eventName))
        {
            return;
        }

        if (eventName == "READY")
        {
            ready_ = true;
            connected_ = true;
            readyEventPending_ = true;
            return;
        }
    }

    void GatewayClient::closeHandles()
    {
        if (webSocket_ != nullptr)
        {
            WinHttpCloseHandle(webSocket_);
            webSocket_ = nullptr;
        }

        if (request_ != nullptr)
        {
            WinHttpCloseHandle(request_);
            request_ = nullptr;
        }

        if (connection_ != nullptr)
        {
            WinHttpCloseHandle(connection_);
            connection_ = nullptr;
        }

        if (session_ != nullptr)
        {
            WinHttpCloseHandle(session_);
            session_ = nullptr;
        }
    }
}