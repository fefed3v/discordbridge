#include "HttpClient.hpp"

#include <vector>

namespace DiscordBridge
{
    namespace
    {
        constexpr wchar_t USER_AGENT[] = L"DiscordBridge/0.0.7";

        void CloseHandle(HINTERNET& handle)
        {
            if (!handle) return;
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

    HttpResponse HttpClient::get(const std::wstring& host, const std::wstring& path, const std::wstring& headers)
    {
        return request(L"GET", host, path, headers, {});
    }

    HttpResponse HttpClient::post(const std::wstring& host, const std::wstring& path, const std::wstring& headers, const std::string& body)
    {
        return request(L"POST", host, path, headers, body);
    }

    HttpResponse HttpClient::put(const std::wstring& host, const std::wstring& path, const std::wstring& headers, const std::string& body)
    {
        return request(L"PUT", host, path, headers, body);
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
        if (session_) return true;

        session_ = WinHttpOpen(
            USER_AGENT,
            direct_ ? WINHTTP_ACCESS_TYPE_NO_PROXY : WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
        );

        if (!session_) return false;

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

    HttpResponse HttpClient::request(const wchar_t* method, const std::wstring& host, const std::wstring& path, const std::wstring& headers, const std::string& body)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        HttpResponse response;

        if (!method || host.empty() || path.empty() || !ensureSession()) return response;

        HINTERNET connection = WinHttpConnect(session_, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
        HINTERNET requestHandle = nullptr;

        if (!connection) return response;

        requestHandle = WinHttpOpenRequest(
            connection,
            method,
            path.c_str(),
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE
        );

        if (!requestHandle)
        {
            CloseHandle(connection);
            return response;
        }

        LPVOID requestData = body.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(body.data());
        const DWORD requestDataSize = static_cast<DWORD>(body.size());

        const wchar_t* headerData = headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headers.c_str();
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
            if (!WinHttpQueryDataAvailable(requestHandle, &available) || available == 0) break;

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
