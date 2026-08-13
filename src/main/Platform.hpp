#pragma once

#include <cstdint>

namespace DiscordBridge
{
    enum class ServerPlatform : std::uint8_t
    {
        Unknown = 0,
        OpenMP = 1,
        SAMP = 2
    };
}