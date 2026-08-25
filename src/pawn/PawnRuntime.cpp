#include "PawnRuntime.hpp"

#include <algorithm>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

namespace DiscordBridge
{
    namespace
    {
        std::string Utf8ToAnsi(const std::string& value)
        {
#ifdef _WIN32
            if (value.empty()) return {};
            const int wideSize = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
            if (wideSize <= 0) return value;
            std::wstring wide(static_cast<std::size_t>(wideSize), L'\0');
            if (MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), wide.data(), wideSize) <= 0) return value;
            const int ansiSize = WideCharToMultiByte(CP_ACP, 0, wide.data(), wideSize, nullptr, 0, nullptr, nullptr);
            if (ansiSize <= 0) return value;
            std::string result(static_cast<std::size_t>(ansiSize), '\0');
            if (WideCharToMultiByte(CP_ACP, 0, wide.data(), wideSize, result.data(), ansiSize, nullptr, nullptr) <= 0) return value;
            return result;
#else
            return value;
#endif
        }

        bool PushPawnString(AMX* amx, const std::string& value, cell& address)
        {
            cell* physical = nullptr;

            if (amx_Allot(amx, static_cast<int>(value.size()) + 1, &address, &physical) != AMX_ERR_NONE) return false;

            if (amx_SetString(physical, value.c_str(), 0, 0, value.size() + 1) == AMX_ERR_NONE) return true;

            amx_Release(amx, address);
            address = 0;

            return false;
        }

        bool FindPublic(AMX* amx, const char* name, int& index)
        {
            return amx && amx_FindPublic(amx, name, &index) == AMX_ERR_NONE;
        }

        void DispatchTwoStrings(const std::vector<AMX*>& scripts, const char* callback, const std::string& first, const std::string& second)
        {
            for (AMX* amx : scripts)
            {
                int index = -1;

                if (!FindPublic(amx, callback, index)) continue;

                cell firstAddress = 0;
                cell secondAddress = 0;

                if (!PushPawnString(amx, first, firstAddress)) continue;

                if (!PushPawnString(amx, second, secondAddress))
                {
                    amx_Release(amx, firstAddress);
                    continue;
                }

                amx_Push(amx, secondAddress);
                amx_Push(amx, firstAddress);

                cell returnValue = 0;
                amx_Exec(amx, &returnValue, index);

                amx_Release(amx, secondAddress);
                amx_Release(amx, firstAddress);
            }
        }

        void DispatchThreeStrings(const std::vector<AMX*>& scripts, const char* callback, const std::string& first, const std::string& second, const std::string& third)
        {
            for (AMX* amx : scripts)
            {
                int index = -1;

                if (!FindPublic(amx, callback, index)) continue;

                cell firstAddress = 0;
                cell secondAddress = 0;
                cell thirdAddress = 0;

                if (!PushPawnString(amx, first, firstAddress)) continue;

                if (!PushPawnString(amx, second, secondAddress))
                {
                    amx_Release(amx, firstAddress);
                    continue;
                }

                if (!PushPawnString(amx, third, thirdAddress))
                {
                    amx_Release(amx, secondAddress);
                    amx_Release(amx, firstAddress);
                    continue;
                }

                amx_Push(amx, thirdAddress);
                amx_Push(amx, secondAddress);
                amx_Push(amx, firstAddress);

                cell returnValue = 0;
                amx_Exec(amx, &returnValue, index);

                amx_Release(amx, thirdAddress);
                amx_Release(amx, secondAddress);
                amx_Release(amx, firstAddress);
            }
        }

        void DispatchStrings(const std::vector<AMX*>& scripts, const char* callback, const std::vector<std::string>& values)
        {
            for (AMX* amx : scripts)
            {
                int index = -1;
                if (!FindPublic(amx, callback, index)) continue;

                std::vector<cell> addresses;
                addresses.reserve(values.size());
                bool ok = true;

                for (const auto& value : values)
                {
                    cell address = 0;
                    if (!PushPawnString(amx, value, address))
                    {
                        ok = false;
                        break;
                    }
                    addresses.push_back(address);
                }

                if (ok)
                {
                    for (auto it = addresses.rbegin(); it != addresses.rend(); ++it) amx_Push(amx, *it);
                    cell returnValue = 0;
                    amx_Exec(amx, &returnValue, index);
                }

                // AMX heap allocations are stack-like. They MUST be released in reverse order.
                for (auto it = addresses.rbegin(); it != addresses.rend(); ++it) amx_Release(amx, *it);
            }
        }

        void DispatchResult(const std::vector<AMX*>& scripts, const char* callback, bool success, const std::string& channelId, const std::string& messageId)
        {
            for (AMX* amx : scripts)
            {
                int index = -1;

                if (!FindPublic(amx, callback, index)) continue;

                cell channelAddress = 0;
                cell messageAddress = 0;

                if (!PushPawnString(amx, channelId, channelAddress)) continue;

                if (!PushPawnString(amx, messageId, messageAddress))
                {
                    amx_Release(amx, channelAddress);
                    continue;
                }

                amx_Push(amx, messageAddress);
                amx_Push(amx, channelAddress);
                amx_Push(amx, success ? 1 : 0);

                cell returnValue = 0;
                amx_Exec(amx, &returnValue, index);

                amx_Release(amx, messageAddress);
                amx_Release(amx, channelAddress);
            }
        }
    }

    bool PawnRuntime::addAMX(AMX* amx)
    {
        if (!amx || std::find(scripts_.begin(), scripts_.end(), amx) != scripts_.end()) return false;

        scripts_.push_back(amx);
        return true;
    }

    bool PawnRuntime::removeAMX(AMX* amx)
    {
        if (!amx) return false;

        const auto iterator = std::find(scripts_.begin(), scripts_.end(), amx);

        if (iterator == scripts_.end()) return false;

        scripts_.erase(iterator);
        return true;
    }

    void PawnRuntime::clear()
    {
        scripts_.clear();
    }

    void PawnRuntime::dispatchReady()
    {
        for (AMX* amx : scripts_)
        {
            int index = -1;

            if (!FindPublic(amx, "DBridge_OnReady", index)) continue;

            cell returnValue = 0;
            amx_Exec(amx, &returnValue, index);
        }
    }

    void PawnRuntime::dispatchMessageCreate(const std::string& userId, const std::string& channelId, const std::string& message)
    {
        DispatchThreeStrings(scripts_, "DBridge_OnMessageCreate", userId, channelId, Utf8ToAnsi(message));
    }

    void PawnRuntime::dispatchGuildMemberAdd(const std::string& guildId, const std::string& userId)
    {
        DispatchTwoStrings(scripts_, "DBridge_OnGuildMemberAdd", guildId, userId);
    }

    void PawnRuntime::dispatchGuildMemberRemove(const std::string& guildId, const std::string& userId)
    {
        DispatchTwoStrings(scripts_, "DBridge_OnGuildMemberRemove", guildId, userId);
    }

    void PawnRuntime::dispatchMessageSent(bool success, const std::string& channelId, const std::string& messageId)
    {
        DispatchResult(scripts_, "DBridge_OnMessageSent", success, channelId, messageId);
    }

    void PawnRuntime::dispatchMessageEdited(bool success, const std::string& channelId, const std::string& messageId)
    {
        DispatchResult(scripts_, "DBridge_OnMessageEdited", success, channelId, messageId);
    }

    void PawnRuntime::dispatchMessageDeleted(bool success, const std::string& channelId, const std::string& messageId)
    {
        DispatchResult(scripts_, "DBridge_OnMessageDeleted", success, channelId, messageId);
    }

    void PawnRuntime::dispatchEmbedSent(bool success, const std::string& channelId, const std::string& messageId)
    {
        DispatchResult(scripts_, "DBridge_OnEmbedSent", success, channelId, messageId);
    }

    void PawnRuntime::dispatchComponentsSent(bool success, const std::string& channelId, const std::string& messageId)
    {
        DispatchResult(scripts_, "DBridge_OnComponentsSent", success, channelId, messageId);
    }

    void PawnRuntime::dispatchButtonClick(const std::string& userId,const std::string& channelId,const std::string& customId,const std::string& interactionId,const std::string& interactionToken)
    { DispatchThreeStrings(scripts_,"DBridge_OnButtonClick",userId,channelId,customId); DispatchStrings(scripts_,"DBridge_OnButtonInteraction",{userId,channelId,customId,interactionId,interactionToken}); }

    void PawnRuntime::dispatchSelectMenu(const std::string& userId,const std::string& channelId,const std::string& customId,const std::string& value,const std::string& interactionId,const std::string& interactionToken)
    { DispatchStrings(scripts_,"DBridge_OnSelectMenu",{userId,channelId,customId,value,interactionId,interactionToken}); }

    void PawnRuntime::dispatchModalSubmit(const std::string& userId, const std::string& channelId, const std::string& customId, const std::vector<std::pair<std::string, std::string>>& values, const std::string& interactionId, const std::string& interactionToken)
    {
        activeModalValues_ = &values;
        DispatchStrings(scripts_, "DBridge_OnModalSubmit", {userId, channelId, customId, interactionId, interactionToken});
        activeModalValues_ = nullptr;
    }

    bool PawnRuntime::getModalValue(const std::string& customId, std::string& value) const
    {
        if (!activeModalValues_ || customId.empty()) return false;
        for (const auto& entry : *activeModalValues_)
        {
            if (entry.first != customId) continue;
            value = entry.second;
            return true;
        }
        return false;
    }

    std::size_t PawnRuntime::size() const
    {
        return scripts_.size();
    }
}