#pragma once
#include <string_view>
#include "../core/Limits.hpp"

using namespace std;

namespace DiscordBridge::InputValidator
{
    inline bool snowflake(string_view value)
    {
        if (value.empty() || value.size() > Limits::MaxSnowflakeLength)
            return false;
        
        for (const char c : value)
            if (c < '0' || c > '9')
                return false;
        
        return true;
    }
    inline bool customId(string_view value) { return !value.empty() && value.size() <= Limits::MaxCustomIdLength; }
    inline bool message(string_view value) { return !value.empty() && value.size() <= Limits::MaxMessageLength; }
    inline bool payload(string_view value) { return !value.empty() && value.size() <= Limits::MaxPayloadLength; }
}