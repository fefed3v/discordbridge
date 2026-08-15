#pragma once

#include "GatewayInfo.hpp"

#include <windows.h>
#include <winhttp.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

namespace DiscordBridge
{
    class GatewayClient final
    {
    private:
        HINTERNET session_{nullptr};
        HINTERNET connection_{nullptr};
        HINTERNET request_{nullptr};
        HINTERNET webSocket_{nullptr};

        std::thread receiveThread_;
        std::thread heartbeatThread_;

        std::mutex heartbeatMutex_;
        std::condition_variable heartbeatCondition_;

        std::atomic_bool initialized_{false};
        std::atomic_bool connected_{false};
        std::atomic_bool running_{false};
        std::atomic_bool ready_{false};
        std::atomic_bool readyEventPending_{false};
        std::atomic_bool heartbeatAck_{true};

        std::atomic<std::int64_t> sequence_{-1};
        std::atomic<std::uint32_t> heartbeatInterval_{0};

        std::string token_;

    public:
        GatewayClient() = default;
        ~GatewayClient();

        GatewayClient(const GatewayClient&) = delete;
        GatewayClient& operator=(const GatewayClient&) = delete;
        GatewayClient(GatewayClient&&) = delete;
        GatewayClient& operator=(GatewayClient&&) = delete;

        bool connect(const GatewayInfo& gatewayInfo, const std::string& token);
        void disconnect();

        bool isInitialized() const;
        bool isConnected() const;
        bool isReady() const;

        bool consumeReadyEvent();

    private:
        void receiveLoop();
        void heartbeatLoop();

        bool receivePayload(std::string& payload);
        bool sendText(const std::string& payload);
        bool sendHeartbeat();
        bool sendIdentify();

        void handlePayload(const std::string& payload);
        void closeHandles();
    };
}