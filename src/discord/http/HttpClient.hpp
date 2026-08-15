#pragma once

#include <string>

namespace DiscordBridge
{
    struct HttpResponse
    {
        unsigned long statusCode{0};
        std::string body;
        bool success{false};
    };

    class HttpClient final
    {
    public:
        HttpClient() = default;
        ~HttpClient() = default;

        HttpResponse get(const std::wstring& host, const std::wstring& path, const std::wstring& headers = L"");
        HttpResponse post(const std::wstring& host, const std::wstring& path, const std::wstring& headers, const std::string& body);
        HttpResponse patch(const std::wstring& host, const std::wstring& path, const std::wstring& headers, const std::string& body);
        HttpResponse del(const std::wstring& host, const std::wstring& path, const std::wstring& headers = L"");

    private:
        HttpResponse request(const wchar_t* method, const std::wstring& host, const std::wstring& path, const std::wstring& headers, const std::string& body);
    };
}