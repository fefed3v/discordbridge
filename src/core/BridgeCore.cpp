#include "BridgeCore.hpp"

#include <utility>

namespace DiscordBridge
{
    BridgeCore::~BridgeCore()
    {
        shutdown();
    }

    bool BridgeCore::initialize(std::unique_ptr<PlatformAdapter> platform)
    {
        if (initialized_ || !platform) return false;
        if (!platform->initialize()) return false;

        platform_ = std::move(platform);
        initialized_ = true;
        return true;
    }

    void BridgeCore::shutdown()
    {
        initialized_ = false;
        discordClient_.shutdown();
        pawnRuntime_.clear();

        if (!platform_) return;
        platform_->shutdown();
        platform_.reset();
    }

    void BridgeCore::process()
    {
        if (!initialized_ || !platform_) return;
        platform_->process();
    }

    bool BridgeCore::isInitialized() const
    {
        return initialized_;
    }

    ServerPlatform BridgeCore::getPlatform() const
    {
        return platform_ ? platform_->getPlatform() : ServerPlatform::Unknown;
    }

    PawnRuntime& BridgeCore::getPawnRuntime()
    {
        return pawnRuntime_;
    }

    const PawnRuntime& BridgeCore::getPawnRuntime() const
    {
        return pawnRuntime_;
    }

    DiscordClient& BridgeCore::getDiscordClient()
    {
        return discordClient_;
    }

    const DiscordClient& BridgeCore::getDiscordClient() const
    {
        return discordClient_;
    }
}
