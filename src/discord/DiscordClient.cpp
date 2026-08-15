#include "DiscordClient.hpp"

#include <windows.h>
#include <utility>

namespace DiscordBridge
{
    static std::wstring Utf8ToWide(const std::string& value)
    {
        if (value.empty()) return {};

        const int size = MultiByteToWideChar(
            CP_UTF8,
            0,
            value.c_str(),
            static_cast<int>(value.size()),
            nullptr,
            0
        );

        if (size <= 0) return {};

        std::wstring result(static_cast<std::size_t>(size), L'\0');

        const int converted = MultiByteToWideChar(
            CP_UTF8,
            0,
            value.c_str(),
            static_cast<int>(value.size()),
            result.data(),
            size
        );

        if (converted <= 0) return {};

        return result;
    }

    DiscordClient::~DiscordClient()
    {
        shutdown();
    }

    bool DiscordClient::initialize(const std::string& token)
    {
        if (initialized_) return false;
        if (token.empty()) return false;

        token_ = token;

        const std::wstring tokenWide = Utf8ToWide(token_);

        if (tokenWide.empty())
        {
            token_.clear();
            return false;
        }

        const std::wstring headers =
            L"Authorization: Bot " + tokenWide + L"\r\n"
            L"Accept: application/json\r\n"
            L"User-Agent: DiscordBridge/0.0.1\r\n";

        const HttpResponse response = httpClient_.get(
            L"discord.com",
            L"/api/v10/gateway/bot",
            headers
        );

        if (!response.success)
        {
            token_.clear();
            return false;
        }

        GatewayInfo gatewayInfo;

        if (!ParseGatewayInfo(response.body, gatewayInfo))
        {
            token_.clear();
            return false;
        }

        gatewayInfo_ = std::move(gatewayInfo);

        if (!gatewayClient_.connect(gatewayInfo_, token_))
        {
            gatewayInfo_ = GatewayInfo{};
            token_.clear();
            return false;
        }

        initialized_ = true;
        connected_ = false;

        return true;
    }

    void DiscordClient::shutdown()
    {
        gatewayClient_.disconnect();
        
        connected_ = false;
        initialized_ = false;

        gatewayInfo_ = GatewayInfo{};

        token_.clear();
    }

    void DiscordClient::process()
    {
        if (!initialized_) return;
    }

    bool DiscordClient::isInitialized() const
    {
        return initialized_;
    }

    bool DiscordClient::isConnected() const
    {
        return gatewayClient_.isReady();
    }

    bool DiscordClient::consumeReadyEvent()
    {
        return gatewayClient_.consumeReadyEvent();
    }

    const std::string& DiscordClient::getToken() const
    {
        return token_;
    }

    const GatewayInfo& DiscordClient::getGatewayInfo() const
    {
        return gatewayInfo_;
    }
}