#include "GatewayInfo.hpp"

#include <cctype>
#include <limits>
#include <string>

namespace DiscordBridge
{
    static bool FindJsonString(const std::string& json, const std::string& key, std::string& value)
    {
        const std::string search = "\"" + key + "\"";

        std::size_t position = json.find(search);
        if (position == std::string::npos) return false;

        position = json.find(':', position + search.size());
        if (position == std::string::npos) return false;

        position = json.find('"', position + 1);
        if (position == std::string::npos) return false;

        ++position;

        const std::size_t end = json.find('"', position);
        if (end == std::string::npos) return false;

        value = json.substr(position, end - position);

        return !value.empty();
    }

    static bool FindJsonInteger(const std::string& json, const std::string& key, int& value, std::size_t startPosition = 0)
    {
        const std::string search = "\"" + key + "\"";

        std::size_t position = json.find(search, startPosition);
        if (position == std::string::npos) return false;

        position = json.find(':', position + search.size());
        if (position == std::string::npos) return false;

        ++position;

        while (position < json.size() && std::isspace(static_cast<unsigned char>(json[position]))) ++position;

        if (position >= json.size()) return false;

        bool negative = false;

        if (json[position] == '-')
        {
            negative = true;
            ++position;
        }

        if (position >= json.size() || !std::isdigit(static_cast<unsigned char>(json[position]))) return false;

        long long result = 0;

        while (position < json.size() && std::isdigit(static_cast<unsigned char>(json[position])))
        {
            result = (result * 10) + (json[position] - '0');

            if (result > std::numeric_limits<int>::max()) return false;

            ++position;
        }

        value = static_cast<int>(negative ? -result : result);

        return true;
    }

    bool GatewayInfo::isValid() const
    {
        return !url.empty() && shards > 0;
    }

    bool ParseGatewayInfo(const std::string& json, GatewayInfo& info)
    {
        if (json.empty()) return false;

        GatewayInfo parsed;

        if (!FindJsonString(json, "url", parsed.url)) return false;

        FindJsonInteger(json, "shards", parsed.shards);

        const std::size_t sessionPosition = json.find("\"session_start_limit\"");

        if (sessionPosition != std::string::npos)
        {
            FindJsonInteger(json, "total", parsed.sessionTotal, sessionPosition);
            FindJsonInteger(json, "remaining", parsed.sessionRemaining, sessionPosition);
            FindJsonInteger(json, "reset_after", parsed.sessionResetAfter, sessionPosition);
            FindJsonInteger(json, "max_concurrency", parsed.maxConcurrency, sessionPosition);
        }

        if (!parsed.isValid()) return false;

        info = std::move(parsed);

        return true;
    }
}