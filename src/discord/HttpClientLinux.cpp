#include "HttpClient.hpp"

#ifndef _WIN32

#include <curl/curl.h>

#include <mutex>
#include <string>
#include <vector>

namespace DiscordBridge
{
    namespace
    {
        std::once_flag curlInitFlag;

        void EnsureCurlGlobalInit()
        {
            std::call_once(curlInitFlag, []() { curl_global_init(CURL_GLOBAL_DEFAULT); });
        }

        std::string Narrow(const std::wstring& value)
        {
            return std::string(value.begin(), value.end());
        }

        size_t WriteCallback(char* data, size_t size, size_t count, void* userData)
        {
            if (!data || !userData) return 0;
            const size_t total = size * count;
            static_cast<std::string*>(userData)->append(data, total);
            return total;
        }

        curl_slist* BuildHeaders(const std::wstring& headers)
        {
            curl_slist* list = nullptr;
            const std::string text = Narrow(headers);
            std::size_t start = 0;

            while (start < text.size())
            {
                std::size_t end = text.find("\r\n", start);
                if (end == std::string::npos) end = text.size();
                if (end > start) list = curl_slist_append(list, text.substr(start, end - start).c_str());
                start = end + 2;
            }

            return list;
        }
    }

    HttpClient::~HttpClient()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        closeSession();
    }

    bool HttpClient::open()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return ensureSession();
    }

    HttpResponse HttpClient::get(const std::wstring& host, const std::wstring& path, const std::wstring& headers)
    {
        return request(L"GET", host, path, headers, {});
    }

    HttpResponse HttpClient::post(const std::wstring& host, const std::wstring& path, const std::wstring& headers, const std::string& body)
    {
        return request(L"POST", host, path, headers, body);
    }

    HttpResponse HttpClient::patch(const std::wstring& host, const std::wstring& path, const std::wstring& headers, const std::string& body)
    {
        return request(L"PATCH", host, path, headers, body);
    }

    HttpResponse HttpClient::del(const std::wstring& host, const std::wstring& path, const std::wstring& headers)
    {
        return request(L"DELETE", host, path, headers, {});
    }

    bool HttpClient::ensureSession()
    {
        EnsureCurlGlobalInit();
        session_ = true;
        return true;
    }

    void HttpClient::closeSession()
    {
        session_ = false;
    }

    HttpResponse HttpClient::request(const wchar_t* method, const std::wstring& host, const std::wstring& path, const std::wstring& headers, const std::string& body)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        HttpResponse response;

        if (!method || host.empty() || path.empty() || !ensureSession()) return response;

        CURL* curl = curl_easy_init();
        if (!curl) return response;

        const std::string url = "https://" + Narrow(host) + Narrow(path);
        const std::string methodText = Narrow(method);
        curl_slist* headerList = BuildHeaders(headers);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "DiscordBridge/0.0.3");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 5000L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, direct_ ? 5000L : 10000L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
        if (headerList) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);

        if (methodText == "POST")
        {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.data());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
        }
        else if (methodText != "GET")
        {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, methodText.c_str());
            if (!body.empty())
            {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.data());
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
            }
        }

        const CURLcode result = curl_easy_perform(curl);
        long statusCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);

        response.statusCode = statusCode > 0 ? static_cast<unsigned long>(statusCode) : 0;
        response.success = result == CURLE_OK && response.statusCode >= 200 && response.statusCode < 300;

        if (headerList) curl_slist_free_all(headerList);
        curl_easy_cleanup(curl);
        return response;
    }
}

#endif
