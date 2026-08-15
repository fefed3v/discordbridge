#pragma once

#include "gateway/GatewayClient.hpp"
#include "gateway/GatewayInfo.hpp"
#include "http/HttpClient.hpp"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

namespace DiscordBridge
{
    class DiscordClient final
    {
    private:
        enum class MessageOperationType
        {
            Send,
            Edit,
            Delete
        };

        struct MessageOperation
        {
            MessageOperationType type{MessageOperationType::Send};
            std::string channelId;
            std::string messageId;
            std::string content;
        };

        struct MessageOperationResult
        {
            MessageOperationType type{MessageOperationType::Send};
            bool success{false};
            std::string channelId;
            std::string messageId;
        };

        std::string token_;

        HttpClient httpClient_;
        GatewayInfo gatewayInfo_;
        GatewayClient gatewayClient_;

        std::mutex outgoingMutex_;
        std::condition_variable outgoingCondition_;
        std::deque<MessageOperation> messageOperations_;

        std::mutex resultMutex_;
        std::deque<MessageOperationResult> messageOperationResults_;

        std::thread restThread_;

        std::atomic_bool initialized_{false};
        std::atomic_bool connected_{false};
        std::atomic_bool restRunning_{false};

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
        bool consumeGuildMemberAddEvent(std::string& guildId, std::string& userId);
        bool consumeGuildMemberRemoveEvent(std::string& guildId, std::string& userId);

        bool consumeMessageSentEvent(bool& success, std::string& channelId, std::string& messageId);
        bool consumeMessageEditedEvent(bool& success, std::string& channelId, std::string& messageId);
        bool consumeMessageDeletedEvent(bool& success, std::string& channelId, std::string& messageId);

        bool setStatus(int status);
        bool setActivity(int type, const std::string& name, const std::string& state = "", const std::string& url = "");
        bool clearActivity();
        bool setPresence(int status, int activityType, const std::string& name, const std::string& state = "", const std::string& url = "", bool afk = false);

        bool sendMessage(const std::string& channelId, const std::string& message);
        bool editMessage(const std::string& channelId, const std::string& messageId, const std::string& content);
        bool deleteMessage(const std::string& channelId, const std::string& messageId);

        const std::string& getToken() const;
        const GatewayInfo& getGatewayInfo() const;

    private:
        void restLoop();

        bool sendMessageRequest(const std::string& channelId, const std::string& message, std::string& messageId);
        bool editMessageRequest(const std::string& channelId, const std::string& messageId, const std::string& content);
        bool deleteMessageRequest(const std::string& channelId, const std::string& messageId);

        bool consumeMessageOperationResult(MessageOperationType type, bool& success, std::string& channelId, std::string& messageId);
    };
}