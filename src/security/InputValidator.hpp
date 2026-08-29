#pragma once
#include <string_view>
#include "../core/Limits.hpp"
namespace DiscordBridge::InputValidator
{
    inline bool snowflake(std::string_view value)
    {
        if (value.empty() || value.size() > Limits::MaxSnowflakeLength)
            return false;
        for (const char c : value)
            if (c < '0' || c > '9')
                return false;
        return true;
    }
    inline bool customId(std::string_view value) { return !value.empty() && value.size() <= Limits::MaxCustomIdLength; }
    inline bool message(std::string_view value) { return !value.empty() && value.size() <= Limits::MaxMessageLength; }
    inline bool payload(std::string_view value) { return !value.empty() && value.size() <= Limits::MaxPayloadLength; }
}
