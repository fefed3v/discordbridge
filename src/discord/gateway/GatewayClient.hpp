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
#include <deque>

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

        struct MessageCreateEvent
        {
            std::string userId;
            std::string channelId;
            std::string message;
        };

        struct GuildMemberEvent
        {
            std::string guildId;
            std::string userId;
        };

        std::mutex eventMutex_;

        std::deque<MessageCreateEvent> messageEvents_;
        std::deque<GuildMemberEvent> guildMemberAddEvents_;
        std::deque<GuildMemberEvent> guildMemberRemoveEvents_;

        int currentStatus_{0};
        int currentActivityType_{0};

        std::string currentActivityName_;
        std::string currentActivityState_;
        std::string currentActivityUrl_;

        bool currentAfk_{false};

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

        bool consumeMessageCreateEvent(std::string& userId, std::string& channelId, std::string& message);

        bool consumeGuildMemberAddEvent(std::string& guildId, std::string& userId);
        bool consumeGuildMemberRemoveEvent(std::string& guildId, std::string& userId);

        bool setStatus(int status);
        bool setActivity(int type, const std::string& name, const std::string& state = "", const std::string& url = "");
        bool clearActivity();
        bool setPresence(int status, int activityType, const std::string& name, const std::string& state = "", const std::string& url = "", bool afk = false);

    private:
        void receiveLoop();
        void heartbeatLoop();

        bool receivePayload(std::string& payload);
        bool sendText(const std::string& payload);
        bool sendHeartbeat();
        bool sendIdentify();
        bool sendPresence();

        void handlePayload(const std::string& payload);
        void closeHandles();
    };
}