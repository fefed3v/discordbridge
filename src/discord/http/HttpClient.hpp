#pragma once

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#endif

#include <mutex>
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
        explicit HttpClient(bool direct = false) : direct_(direct) {}
        ~HttpClient();

        bool open();

        HttpClient(const HttpClient&) = delete;
        HttpClient& operator=(const HttpClient&) = delete;
        HttpClient(HttpClient&&) = delete;
        HttpClient& operator=(HttpClient&&) = delete;

        HttpResponse get(const std::wstring& host, const std::wstring& path, const std::wstring& headers = L"");
        HttpResponse post(const std::wstring& host, const std::wstring& path, const std::wstring& headers, const std::string& body);
        HttpResponse put(const std::wstring& host, const std::wstring& path, const std::wstring& headers, const std::string& body = {});
        HttpResponse patch(const std::wstring& host, const std::wstring& path, const std::wstring& headers, const std::string& body);
        HttpResponse del(const std::wstring& host, const std::wstring& path, const std::wstring& headers = L"");

    private:
        bool ensureSession();
        void closeSession();
        HttpResponse request(const wchar_t* method, const std::wstring& host, const std::wstring& path, const std::wstring& headers, const std::string& body);

#ifdef _WIN32
        HINTERNET session_{nullptr};
#else
        bool session_{false};
#endif
        bool direct_{false};
        std::mutex mutex_;
    };
}
