#pragma once

#include <sdk.hpp>
#include <Server/Components/Pawn/pawn.hpp>

#include "../OpenMPAdapter.hpp"
#include "../../../core/BridgeCore.hpp"

#include <memory>

namespace DiscordBridge
{
    class DiscordBridgeComponent final :
        public IComponent,
        public CoreEventHandler,
        public PawnEventHandler
    {
    public:
        DiscordBridgeComponent() = default;
        ~DiscordBridgeComponent() override = default;

        StringView componentName() const override;
        SemanticVersion componentVersion() const override;

        UID getUID() override;

        void onLoad(ICore* core) override;
        void onInit(IComponentList* components) override;
        void onReady() override;

        void onTick(Microseconds elapsed, TimePoint now) override;

        void onAmxLoad(IPawnScript& script) override;
        void onAmxUnload(IPawnScript& script) override;

        void free() override;
        void reset() override;

    private:
        ICore* core_ = nullptr;
        IPawnComponent* pawn_ = nullptr;

        BridgeCore bridgeCore_;

        bool initialized_ = false;
    };
}