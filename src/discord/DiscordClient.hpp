#pragma once

#include "Gateway.hpp"
#include "HttpClient.hpp"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

namespace DiscordBridge
{
    class DiscordClient
    {
    private:
        enum class MessageOperationType
        {
            Send,
            Edit,
            Delete,
            SendEmbed,
            SendComponents,
            AcknowledgeInteraction
        };

        struct MessageOperation
        {
            MessageOperationType type;
            std::string channelId;
            std::string messageId;
            std::string content;
        };

        struct MessageOperationResult
        {
            MessageOperationType type;
            bool success;
            std::string channelId;
            std::string messageId;
        };

        struct InteractionOperation
        {
            std::string interactionId;
            std::string interactionToken;
            std::string body;
        };

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
        bool consumeButtonClickEvent(std::string& interactionId, std::string& interactionToken, std::string& userId, std::string& channelId, std::string& customId);
        bool consumeSelectMenuEvent(std::string& interactionId, std::string& interactionToken, std::string& userId, std::string& channelId, std::string& customId, std::string& value);
        bool consumeModalEvent(std::string& interactionId, std::string& interactionToken, std::string& userId, std::string& channelId, std::string& customId, std::vector<std::pair<std::string, std::string>>& values);
        bool acknowledgeInteractionAsync(const std::string& interactionId, const std::string& interactionToken);
        bool deferInteractionAsync(const std::string& interactionId, const std::string& interactionToken, bool ephemeral = true);
        bool respondInteraction(const std::string& interactionId, const std::string& interactionToken, const std::string& content, bool ephemeral = true);
        bool showModal(const std::string& interactionId, const std::string& interactionToken, const std::string& modalJson);

        bool consumeMessageSentEvent(bool& success, std::string& channelId, std::string& messageId);
        bool consumeMessageEditedEvent(bool& success, std::string& channelId, std::string& messageId);
        bool consumeMessageDeletedEvent(bool& success, std::string& channelId, std::string& messageId);
        bool consumeEmbedSentEvent(bool& success, std::string& channelId, std::string& messageId);
        bool consumeComponentsSentEvent(bool& success, std::string& channelId, std::string& messageId);

        bool setStatus(int status);
        bool setActivity(int type, const std::string& name, const std::string& state = "", const std::string& url = "");
        bool clearActivity();
        bool setPresence(int status, int activityType, const std::string& name, const std::string& state = "", const std::string& url = "", bool afk = false);

        bool sendMessage(const std::string& channelId, const std::string& message);
        bool editMessage(const std::string& channelId, const std::string& messageId, const std::string& content);
        bool deleteMessage(const std::string& channelId, const std::string& messageId);
        bool sendEmbed(const std::string& channelId, const std::string& embedJson);
        bool sendComponents(const std::string& channelId, const std::string& componentsJson);

        const std::string& getToken() const;
        const GatewayInfo& getGatewayInfo() const;

    private:
        void restLoop();
        void interactionLoop();
        bool enqueueInteractionOperation(InteractionOperation operation, bool prioritize = false);
        bool sendInteractionCallback(const std::string& interactionId, const std::string& interactionToken, const std::string& body);
        bool enqueueMessageOperation(MessageOperation operation, bool prioritize = false);

        bool consumeMessageOperationResult(MessageOperationType type, bool& success, std::string& channelId, std::string& messageId);

        bool sendMessageRequest(const std::string& channelId, const std::string& message, std::string& messageId);
        bool editMessageRequest(const std::string& channelId, const std::string& messageId, const std::string& content);
        bool deleteMessageRequest(const std::string& channelId, const std::string& messageId);
        bool sendEmbedRequest(const std::string& channelId, const std::string& embedJson, std::string& messageId);
        bool sendComponentsRequest(const std::string& channelId, const std::string& componentsJson, std::string& messageId);
        bool acknowledgeInteraction(const std::string& interactionId, const std::string& interactionToken);

        HttpClient httpClient_;
        HttpClient interactionHttpClient_{true};
        Gateway gateway_;
        GatewayInfo gatewayInfo_;

        std::thread restThread_;
        std::thread interactionThread_;

        std::mutex outgoingMutex_;
        std::mutex resultMutex_;
        std::mutex interactionMutex_;
        std::mutex interactionResponseMutex_;
        std::condition_variable outgoingCondition_;
        std::condition_variable interactionCondition_;

        std::deque<MessageOperation> messageOperations_;
        std::deque<MessageOperationResult> messageOperationResults_;
        std::deque<InteractionOperation> interactionOperations_;
        std::unordered_set<std::string> interactionResponses_;

        std::atomic_bool initialized_{false};
        std::atomic_bool connected_{false};
        std::atomic_bool restRunning_{false};
        std::atomic_bool interactionRunning_{false};

        std::string token_;
    };
}