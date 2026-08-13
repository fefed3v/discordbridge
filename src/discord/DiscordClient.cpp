#include "DiscordClient.hpp"

#include <dpp/dpp.h>
#include <exception>
#include <iostream>

namespace DiscordBridge
{
    DiscordClient::DiscordClient() = default;

    DiscordClient::~DiscordClient()
    {
        shutdown();
    }

    bool DiscordClient::initialize(const std::string& token)
    {
        if (initialized_.load() || token.empty()) return false;

        try
        {
            connected_.store(false);
            cluster_ = std::make_unique<dpp::cluster>(token);

            cluster_->on_ready([this](const dpp::ready_t& event)
            {
                (void)event;
                connected_.store(true);
                std::cout << "[DiscordBridge] Discord connected as " << cluster_->me.username << std::endl;
            });

            cluster_->on_log([](const dpp::log_t& event)
            {
                std::cout << "[DPP] " << event.message << std::endl;
            });

            initialized_.store(true);
            cluster_->start(dpp::st_return);
            return true;
        }
        catch (const std::exception& exception)
        {
            connected_.store(false);
            initialized_.store(false);
            cluster_.reset();
            std::cerr << "[DiscordBridge] Failed to initialize Discord: " << exception.what() << std::endl;
            return false;
        }
        catch (...)
        {
            connected_.store(false);
            initialized_.store(false);
            cluster_.reset();
            std::cerr << "[DiscordBridge] Failed to initialize Discord: unknown exception." << std::endl;
            return false;
        }
    }

    void DiscordClient::shutdown()
    {
        connected_.store(false);
        initialized_.store(false);

        if (!cluster_) return;

        try
        {
            cluster_->shutdown();
        }
        catch (const std::exception& exception)
        {
            std::cerr << "[DiscordBridge] Failed to shutdown Discord cleanly: " << exception.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "[DiscordBridge] Failed to shutdown Discord cleanly: unknown exception." << std::endl;
        }

        cluster_.reset();
    }

    bool DiscordClient::isInitialized() const
    {
        return initialized_.load();
    }

    bool DiscordClient::isConnected() const
    {
        return connected_.load();
    }
}
