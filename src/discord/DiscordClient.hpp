#pragma once

#include <atomic>
#include <memory>
#include <string>

namespace dpp
{
    class cluster;
}

namespace DiscordBridge
{
    class DiscordClient final
    {
    private:
        std::unique_ptr<dpp::cluster> cluster_;
        std::atomic_bool initialized_{false};
        std::atomic_bool connected_{false};

    public:
        DiscordClient();
        ~DiscordClient();

        DiscordClient(const DiscordClient&) = delete;
        DiscordClient& operator=(const DiscordClient&) = delete;
        DiscordClient(DiscordClient&&) = delete;
        DiscordClient& operator=(DiscordClient&&) = delete;

        bool initialize(const std::string& token);
        void shutdown();

        bool isInitialized() const;
        bool isConnected() const;
    };
}