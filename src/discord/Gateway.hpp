#pragma once

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#endif

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include "../security/AbuseGuard.hpp"

namespace DiscordBridge
{
    struct GatewayInfo
    {
        std::string url;
        int shards{1};
        int sessionTotal{0};
        int sessionRemaining{0};
        int sessionResetAfter{0};
        int maxConcurrency{1};

        bool isValid() const;
    };

    bool ParseGatewayInfo(const std::string &json, GatewayInfo &info);

    class Gateway final
    {
    private:
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

        struct SlashCommandEvent
        {
            std::string interactionId;
            std::string interactionToken;
            std::string userId;
            std::string guildId;
            std::string channelId;
            std::string commandName;
            std::vector<std::pair<std::string, std::string>> options;
        };

        struct ComponentEvent
        {
            std::string interactionId;
            std::string interactionToken;
            std::string userId;
            std::string channelId;
            std::string customId;
            std::string value;
            std::vector<std::pair<std::string, std::string>> modalValues;
        };

#ifdef _WIN32
        HINTERNET session_{nullptr};
        HINTERNET connection_{nullptr};
        HINTERNET request_{nullptr};
        HINTERNET webSocket_{nullptr};
#else
        void *webSocket_{nullptr};
#endif

        std::thread receiveThread_;
        std::thread heartbeatThread_;

        std::mutex heartbeatMutex_;
        std::condition_variable heartbeatCondition_;

        std::mutex sendMutex_;
        std::mutex presenceMutex_;
        std::mutex eventMutex_;
        mutable std::mutex applicationMutex_;
        std::deque<MessageCreateEvent> messageEvents_;
        std::deque<GuildMemberEvent> guildMemberAddEvents_;
        std::deque<GuildMemberEvent> guildMemberRemoveEvents_;
        std::deque<ComponentEvent> buttonClickEvents_;
        std::deque<ComponentEvent> selectMenuEvents_;
        std::deque<ComponentEvent> modalEvents_;
        std::deque<SlashCommandEvent> slashCommandEvents_;
        std::deque<SlashCommandEvent> autocompleteEvents_;

        std::atomic_bool initialized_{false};
        std::atomic_bool connected_{false};
        std::atomic_bool running_{false};
        std::atomic_bool ready_{false};
        std::atomic_bool readyEventPending_{false};
        std::atomic_bool heartbeatAck_{true};

        std::atomic<std::int64_t> sequence_{-1};
        std::atomic<std::uint32_t> heartbeatInterval_{0};

        int currentStatus_{0};
        int currentActivityType_{0};
        bool currentAfk_{false};

        std::string currentActivityName_;
        std::string currentActivityState_;
        std::string currentActivityUrl_;
        std::string token_;
        std::string applicationId_;
        AbuseGuard abuseGuard_;

    public:
        Gateway() = default;
        ~Gateway();

        Gateway(const Gateway &) = delete;
        Gateway &operator=(const Gateway &) = delete;
        Gateway(Gateway &&) = delete;
        Gateway &operator=(Gateway &&) = delete;

        bool connect(const GatewayInfo &gatewayInfo, const std::string &token);
        void disconnect();

        bool isInitialized() const;
        bool isConnected() const;
        bool isReady() const;

        bool consumeReadyEvent();
        bool consumeMessageCreateEvent(std::string &userId, std::string &channelId, std::string &message);
        bool consumeGuildMemberAddEvent(std::string &guildId, std::string &userId);
        bool consumeGuildMemberRemoveEvent(std::string &guildId, std::string &userId);
        bool consumeButtonClickEvent(std::string &interactionId, std::string &interactionToken, std::string &userId, std::string &channelId, std::string &customId);
        bool consumeSelectMenuEvent(std::string &interactionId, std::string &interactionToken, std::string &userId, std::string &channelId, std::string &customId, std::string &value);
        bool consumeModalEvent(std::string &interactionId, std::string &interactionToken, std::string &userId, std::string &channelId, std::string &customId, std::vector<std::pair<std::string, std::string>> &values);
        bool consumeSlashCommandEvent(std::string &interactionId, std::string &interactionToken, std::string &userId, std::string &guildId, std::string &channelId, std::string &commandName, std::vector<std::pair<std::string, std::string>> &options);
        bool consumeAutocompleteEvent(std::string &interactionId, std::string &interactionToken, std::string &userId, std::string &guildId, std::string &channelId, std::string &commandName, std::vector<std::pair<std::string, std::string>> &options);
        std::string getApplicationId() const;

        bool setStatus(int status);
        bool setActivity(int type, const std::string &name, const std::string &state = "", const std::string &url = "");
        bool clearActivity();
        bool setPresence(int status, int activityType, const std::string &name, const std::string &state = "", const std::string &url = "", bool afk = false);

    private:
        void receiveLoop();
        void heartbeatLoop();

        void handlePayload(const std::string &payload);
        void handleHello(const std::string &payload);
        void handleDispatch(const std::string &payload);

        bool receivePayload(std::string &payload);
        bool sendText(const std::string &payload);
        bool sendHeartbeat();
        bool sendIdentify();
        bool sendPresence();

        bool consumeGuildMemberEvent(std::deque<GuildMemberEvent> &events, std::string &guildId, std::string &userId);
        bool failConnection();

        void stopConnection();
        void closeHandles();
        void resetState();
    };
}