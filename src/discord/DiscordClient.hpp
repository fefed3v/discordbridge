#pragma once

#include "http/HttpClient.hpp"
#include "gateway/GatewayInfo.hpp"
#include "gateway/GatewayClient.hpp"

#include <atomic>
#include <string>

namespace DiscordBridge
{
    class DiscordClient final
    {
    private:
        std::string token_;
        HttpClient httpClient_;
        GatewayInfo gatewayInfo_;
        GatewayClient gatewayClient_;

        std::atomic_bool initialized_{false};
        std::atomic_bool connected_{false};

    public:
        DiscordClient() = default;
        ~DiscordClient();

        DiscordClient(const DiscordClient&) = delete;
        DiscordClient& operator=(const DiscordClient&) = delete;
        DiscordClient(DiscordClient&&) = delete;
        DiscordClient& operator=(DiscordClient&&) = delete;

        bool initialize(const std::string& token);
        void shutdown();
        void process();

        bool isInitialized() const;
        bool isConnected() const;
        bool consumeReadyEvent();

        bool consumeMessageCreateEvent(std::string& userId, std::string& channelId, std::string& message);

        const std::string& getToken() const;
        const GatewayInfo& getGatewayInfo() const;
    };
}