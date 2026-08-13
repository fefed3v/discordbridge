#include "Component.hpp"

#include "../../../main/Version.hpp"
#include "../../../pawn/natives/VersionNatives.hpp"

#include <memory>
#include <utility>

namespace DiscordBridge
{
    StringView DiscordBridgeComponent::componentName() const
    {
        return "DiscordBridge";
    }

    SemanticVersion DiscordBridgeComponent::componentVersion() const
    {
        return SemanticVersion(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, 0);
    }

    UID DiscordBridgeComponent::getUID()
    {
        return UID(0x20826DD87DD56B03);
    }

    void DiscordBridgeComponent::onLoad(ICore* core)
    {
        core_ = core;
        if (core_ == nullptr) return;

        core_->getEventDispatcher().addEventHandler(this);
        core_->printLn("[DiscordBridge] Loading Patch %s...", VERSION_STRING);
    }

    void DiscordBridgeComponent::onInit(IComponentList* components)
    {
        if (initialized_) return;
        if (core_ == nullptr)
        {
            return;
        }
        if (components == nullptr)
        {
            core_->printLn("[DiscordBridge] Component list is null.");
            return;
        }

        pawn_ = components->queryComponent<IPawnComponent>();
        if (pawn_ == nullptr)
        {
            core_->printLn("[DiscordBridge] Pawn component not found.");
            return;
        }

        pawn_->getEventDispatcher().addEventHandler(this);
        auto adapter = std::make_unique<OpenMPAdapter>();

        if (!bridgeCore_.initialize(std::move(adapter)))
        {
            core_->printLn("[DiscordBridge] Failed to initialize BridgeCore.");
            pawn_->getEventDispatcher().removeEventHandler(this);
            pawn_ = nullptr;
            return;
        }

        initialized_ = true;
        core_->printLn("[DiscordBridge] BridgeCore initialized.");
        core_->printLn("[DiscordBridge] Platform: open.mp.");
    }

    void DiscordBridgeComponent::onReady()
    {
        if (initialized_ && core_ != nullptr) core_->printLn("[DiscordBridge] open.mp server ready.");
    }

    void DiscordBridgeComponent::onTick(Microseconds elapsed, TimePoint now)
    {
        (void)elapsed;
        (void)now;
        if (initialized_) bridgeCore_.process();
    }

    void DiscordBridgeComponent::onAmxLoad(IPawnScript& script)
    {
        if (!initialized_) return;

        AMX* amx = script.GetAMX();
        if (amx == nullptr) return;

        if (!bridgeCore_.getPawnRuntime().addAMX(amx))
        {
            if (core_ != nullptr) core_->printLn("[DiscordBridge] AMX already registered.");
            return;
        }

        const int result = RegisterVersionNativesOpenMP(script);
        if (result != AMX_ERR_NONE)
        {
            bridgeCore_.getPawnRuntime().removeAMX(amx);
            if (core_ != nullptr) core_->printLn("[DiscordBridge] Failed to register version natives. Error: %d", result);
            return;
        }

        if (core_ != nullptr) core_->printLn("[DiscordBridge] Pawn script registered.");
    }

    void DiscordBridgeComponent::onAmxUnload(IPawnScript& script)
    {
        AMX* amx = script.GetAMX();
        if (amx == nullptr) return;

        bridgeCore_.getPawnRuntime().removeAMX(amx);
        if (core_ != nullptr) core_->printLn("[DiscordBridge] Pawn script unregistered.");
    }

    void DiscordBridgeComponent::reset()
    {
        initialized_ = false;
        bridgeCore_.shutdown();

        if (pawn_ != nullptr)
        {
            pawn_->getEventDispatcher().removeEventHandler(this);
            pawn_ = nullptr;
        }
    }

    void DiscordBridgeComponent::free()
    {
        reset();

        if (core_ != nullptr)
        {
            core_->getEventDispatcher().removeEventHandler(this);
            core_ = nullptr;
        }

        delete this;
    }
}
