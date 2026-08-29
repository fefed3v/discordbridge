#include "HttpClient.hpp"

#include <vector>
#ifndef _WIN32
#include <curl/curl.h>
#include <codecvt>
#include <locale>
#endif

#ifdef _WIN32
namespace DiscordBridge
{
    namespace
    {
        constexpr wchar_t USER_AGENT[] = L"DiscordBridge/0.0.9";

        void CloseHandle(HINTERNET &handle)
        {
            if (!handle)
                return;
            WinHttpCloseHandle(handle);
            handle = nullptr;
        }
    }

    bool HttpClient::open()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return ensureSession();
    }

    HttpClient::~HttpClient()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        closeSession();
    }

    HttpResponse HttpClient::get(const std::wstring &host, const std::wstring &path, const std::wstring &headers)
    {
        return request(L"GET", host, path, headers, {});
    }

    HttpResponse HttpClient::post(const std::wstring &host, const std::wstring &path, const std::wstring &headers, const std::string &body)
    {
        return request(L"POST", host, path, headers, body);
    }

    HttpResponse HttpClient::put(const std::wstring &host, const std::wstring &path, const std::wstring &headers, const std::string &body)
    {
        return request(L"PUT", host, path, headers, body);
    }

    HttpResponse HttpClient::patch(const std::wstring &host, const std::wstring &path, const std::wstring &headers, const std::string &body)
    {
        return request(L"PATCH", host, path, headers, body);
    }

    HttpResponse HttpClient::del(const std::wstring &host, const std::wstring &path, const std::wstring &headers)
    {
        return request(L"DELETE", host, path, headers, {});
    }

    bool HttpClient::ensureSession()
    {
        if (session_)
            return true;

        session_ = WinHttpOpen(
            USER_AGENT,
            direct_ ? WINHTTP_ACCESS_TYPE_NO_PROXY : WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);

        if (!session_)
            return false;

        if (!WinHttpSetTimeouts(session_, 5000, 5000, 10000, 10000))
        {
            closeSession();
            return false;
        }

        return true;
    }

    void HttpClient::closeSession()
    {
        CloseHandle(session_);
    }

    HttpResponse HttpClient::request(const wchar_t *method, const std::wstring &host, const std::wstring &path, const std::wstring &headers, const std::string &body)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        HttpResponse response;

        if (!method || host.empty() || path.empty() || !ensureSession())
            return response;

        HINTERNET connection = WinHttpConnect(session_, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
        HINTERNET requestHandle = nullptr;

        if (!connection)
            return response;

        requestHandle = WinHttpOpenRequest(
            connection,
            method,
            path.c_str(),
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE);

        if (!requestHandle)
        {
            CloseHandle(connection);
            return response;
        }

        LPVOID requestData = body.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char *>(body.data());
        const DWORD requestDataSize = static_cast<DWORD>(body.size());

        const wchar_t *headerData = headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headers.c_str();
        const DWORD headerSize = headers.empty() ? 0 : static_cast<DWORD>(-1L);

        if (!WinHttpSendRequest(requestHandle, headerData, headerSize, requestData, requestDataSize, requestDataSize, 0) ||
            !WinHttpReceiveResponse(requestHandle, nullptr))
        {
            CloseHandle(requestHandle);
            CloseHandle(connection);
            return response;
        }

        DWORD statusCode = 0;
        DWORD statusCodeSize = sizeof(statusCode);

        if (WinHttpQueryHeaders(
                requestHandle,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,
                &statusCode,
                &statusCodeSize,
                WINHTTP_NO_HEADER_INDEX))
        {
            response.statusCode = statusCode;
        }

        constexpr DWORD READ_CHUNK_SIZE = 16 * 1024;
        std::vector<char> buffer(READ_CHUNK_SIZE);

        while (true)
        {
            DWORD available = 0;
            if (!WinHttpQueryDataAvailable(requestHandle, &available) || available == 0)
                break;

            while (available > 0)
            {
                const DWORD toRead = (available < READ_CHUNK_SIZE) ? available : READ_CHUNK_SIZE;
                DWORD bytesRead = 0;

                if (!WinHttpReadData(requestHandle, buffer.data(), toRead, &bytesRead) || bytesRead == 0)
                {
                    available = 0;
                    break;
                }

                response.body.append(buffer.data(), bytesRead);
                available -= bytesRead;
            }
        }

        response.success = response.statusCode >= 200 && response.statusCode < 300;

        CloseHandle(requestHandle);
        CloseHandle(connection);

        return response;
    }
}

#else

namespace DiscordBridge
{
    namespace
    {
        std::string Narrow(const std::wstring &value)
        {
            std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
            return convert.to_bytes(value);
        }
        size_t WriteCallback(char *data, size_t size, size_t count, void *userdata)
        {
            auto *output = static_cast<std::string *>(userdata);
            output->append(data, size * count);
            return size * count;
        }
    }

    bool HttpClient::open()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return ensureSession();
    }

    HttpClient::~HttpClient()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        closeSession();
    }

    HttpResponse HttpClient::get(const std::wstring &h, const std::wstring &p, const std::wstring &x) { return request(L"GET", h, p, x, {}); }
    HttpResponse HttpClient::post(const std::wstring &h, const std::wstring &p, const std::wstring &x, const std::string &b) { return request(L"POST", h, p, x, b); }
    HttpResponse HttpClient::put(const std::wstring &h, const std::wstring &p, const std::wstring &x, const std::string &b) { return request(L"PUT", h, p, x, b); }
    HttpResponse HttpClient::patch(const std::wstring &h, const std::wstring &p, const std::wstring &x, const std::string &b) { return request(L"PATCH", h, p, x, b); }
    HttpResponse HttpClient::del(const std::wstring &h, const std::wstring &p, const std::wstring &x) { return request(L"DELETE", h, p, x, {}); }

    bool HttpClient::ensureSession()
    {
        if (session_)
            return true;
        session_ = curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
        return session_;
    }
    void HttpClient::closeSession() { session_ = false; }

    HttpResponse HttpClient::request(const wchar_t *method, const std::wstring &host, const std::wstring &path, const std::wstring &headers, const std::string &body)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        HttpResponse response;
        if (!method || host.empty() || path.empty() || !ensureSession())
            return response;
        CURL *curl = curl_easy_init();
        if (!curl)
            return response;
        const std::string url = "https://" + Narrow(host) + Narrow(path);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "DiscordBridge/0.0.9");
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 5000L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 10000L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
        const std::string methodText = Narrow(method);
        if (methodText != "GET")
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, methodText.c_str());
        if (!body.empty())
        {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.data());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
        }
        curl_slist *list = nullptr;
        std::string hs = Narrow(headers), line;
        std::size_t pos = 0;
        while (pos < hs.size())
        {
            auto end = hs.find("\r\n", pos);
            line = hs.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
            if (!line.empty())
                list = curl_slist_append(list, line.c_str());
            if (end == std::string::npos)
                break;
            pos = end + 2;
        }
        if (list)
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
        const CURLcode code = curl_easy_perform(curl);
        long status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        response.statusCode = static_cast<unsigned long>(status);
        response.success = code == CURLE_OK && status >= 200 && status < 300;
        if (list)
            curl_slist_free_all(list);
        curl_easy_cleanup(curl);
        return response;
    }
}

#endif
