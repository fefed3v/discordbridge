#pragma once
#include <cstddef>

using namespace std;

namespace DiscordBridge::Limits
{
    inline constexpr size_t MaxRestQueue = 2048;
    inline constexpr size_t MaxInteractionQueue = 1024;
    inline constexpr size_t MaxResultQueue = 4096;
    inline constexpr size_t MaxGatewayMessages = 2048;
    inline constexpr size_t MaxGatewayMembers = 1024;
    inline constexpr size_t MaxGatewayInteractions = 2048;
    inline constexpr size_t MaxEventsPerTick = 160;
    inline constexpr size_t MaxCriticalPerTick = 80;
    inline constexpr size_t MaxMessageLength = 2000;
    inline constexpr size_t MaxCustomIdLength = 100;
    inline constexpr size_t MaxSnowflakeLength = 20;
    inline constexpr size_t MaxPayloadLength = 1024 * 1024;
}
