#include "HttpClient.hpp"

#include <windows.h>
#include <winhttp.h>

#include <vector>

namespace DiscordBridge
{
    HttpResponse HttpClient::get(const std::wstring& host, const std::wstring& path, const std::wstring& headers)
    {
        return request(L"GET", host, path, headers, "");
    }

    HttpResponse HttpClient::post(const std::wstring& host, const std::wstring& path, const std::wstring& headers, const std::string& body)
    {
        return request(L"POST", host, path, headers, body);
    }

    HttpResponse HttpClient::request(const wchar_t* method, const std::wstring& host, const std::wstring& path, const std::wstring& headers, const std::string& body)
    {
        HttpResponse response;

        HINTERNET session = WinHttpOpen(
            L"DiscordBridge/0.0.1",
            WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
        );

        if (session == nullptr) return response;

        WinHttpSetTimeouts(session, 5000, 5000, 10000, 10000);

        HINTERNET connection = WinHttpConnect(session, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);

        if (connection == nullptr)
        {
            WinHttpCloseHandle(session);
            return response;
        }

        HINTERNET requestHandle = WinHttpOpenRequest(
            connection,
            method,
            path.c_str(),
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE
        );

        if (requestHandle == nullptr)
        {
            WinHttpCloseHandle(connection);
            WinHttpCloseHandle(session);
            return response;
        }

        LPVOID requestData = WINHTTP_NO_REQUEST_DATA;
        DWORD requestDataSize = 0;

        if (!body.empty())
        {
            requestData = const_cast<char*>(body.data());
            requestDataSize = static_cast<DWORD>(body.size());
        }

        const wchar_t* headerData = headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headers.c_str();
        DWORD headerSize = headers.empty() ? 0 : static_cast<DWORD>(-1L);

        if (!WinHttpSendRequest(
            requestHandle,
            headerData,
            headerSize,
            requestData,
            requestDataSize,
            requestDataSize,
            0
        ))
        {
            WinHttpCloseHandle(requestHandle);
            WinHttpCloseHandle(connection);
            WinHttpCloseHandle(session);
            return response;
        }

        if (!WinHttpReceiveResponse(requestHandle, nullptr))
        {
            WinHttpCloseHandle(requestHandle);
            WinHttpCloseHandle(connection);
            WinHttpCloseHandle(session);
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
            WINHTTP_NO_HEADER_INDEX
        ))
        {
            response.statusCode = statusCode;
        }

        while (true)
        {
            DWORD available = 0;

            if (!WinHttpQueryDataAvailable(requestHandle, &available)) break;
            if (available == 0) break;

            std::vector<char> buffer(static_cast<std::size_t>(available));

            DWORD bytesRead = 0;

            if (!WinHttpReadData(requestHandle, buffer.data(), available, &bytesRead)) break;
            if (bytesRead == 0) break;

            response.body.append(buffer.data(), bytesRead);
        }

        response.success = response.statusCode >= 200 && response.statusCode < 300;

        WinHttpCloseHandle(requestHandle);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);

        return response;
    }

    HttpResponse HttpClient::patch(const std::wstring& host, const std::wstring& path, const std::wstring& headers, const std::string& body)
    {
        return request(L"PATCH", host, path, headers, body);
    }

    HttpResponse HttpClient::del(const std::wstring& host, const std::wstring& path, const std::wstring& headers)
    {
        return request(L"DELETE", host, path, headers, "");
    }
}