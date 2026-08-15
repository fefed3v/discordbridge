#include "PawnRuntime.hpp"

#include <algorithm>
#include <windows.h>

namespace DiscordBridge
{
    static std::string Utf8ToAnsi(const std::string& value)
    {
        if (value.empty()) return {};

        const int wideSize = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
        if (wideSize <= 0) return value;

        std::wstring wide(static_cast<std::size_t>(wideSize), L'\0');

        if (MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), wide.data(), wideSize) <= 0) return value;

        const int ansiSize = WideCharToMultiByte(CP_ACP, 0, wide.data(), wideSize, nullptr, 0, nullptr, nullptr);
        if (ansiSize <= 0) return value;

        std::string result(static_cast<std::size_t>(ansiSize), '\0');

        if (WideCharToMultiByte(CP_ACP, 0, wide.data(), wideSize, result.data(), ansiSize, nullptr, nullptr) <= 0) return value;

        return result;
    }
    
    bool PawnRuntime::addAMX(AMX* amx)
    {
        if (amx == nullptr) return false;

        const auto iterator = std::find(scripts_.begin(), scripts_.end(), amx);

        if (iterator != scripts_.end()) return false;

        scripts_.push_back(amx);

        return true;
    }

    bool PawnRuntime::removeAMX(AMX* amx)
    {
        if (amx == nullptr) return false;

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
            if (amx == nullptr) continue;

            int index = -1;

            if (amx_FindPublic(amx, "DBridge_OnReady", &index) != AMX_ERR_NONE) continue;

            cell returnValue = 0;

            amx_Exec(amx, &returnValue, index);
        }
    }

    void PawnRuntime::dispatchMessageCreate(const std::string& userId, const std::string& channelId, const std::string& message)
    {
        const std::string pawnMessage = Utf8ToAnsi(message);

        for (AMX* amx : scripts_)
        {
            if (amx == nullptr) continue;

            int index = -1;

            if (amx_FindPublic(amx, "DBridge_OnMessageCreate", &index) != AMX_ERR_NONE) continue;

            cell userAddress = 0;
            cell channelAddress = 0;
            cell messageAddress = 0;

            cell* userPhysical = nullptr;
            cell* channelPhysical = nullptr;
            cell* messagePhysical = nullptr;

            if (amx_Allot(amx, static_cast<int>(userId.size()) + 1, &userAddress, &userPhysical) != AMX_ERR_NONE) continue;

            if (amx_Allot(amx, static_cast<int>(channelId.size()) + 1, &channelAddress, &channelPhysical) != AMX_ERR_NONE)
            {
                amx_Release(amx, userAddress);
                continue;
            }

            if (amx_Allot(amx, static_cast<int>(pawnMessage.size()) + 1, &messageAddress, &messagePhysical) != AMX_ERR_NONE)
            {
                amx_Release(amx, channelAddress);
                amx_Release(amx, userAddress);
                continue;
            }

            amx_SetString(userPhysical, userId.c_str(), 0, 0, userId.size() + 1);
            amx_SetString(channelPhysical, channelId.c_str(), 0, 0, channelId.size() + 1);
            amx_SetString(messagePhysical, pawnMessage.c_str(), 0, 0, pawnMessage.size() + 1);

            amx_Push(amx, messageAddress);
            amx_Push(amx, channelAddress);
            amx_Push(amx, userAddress);

            cell returnValue = 0;
            amx_Exec(amx, &returnValue, index);

            amx_Release(amx, messageAddress);
            amx_Release(amx, channelAddress);
            amx_Release(amx, userAddress);
        }
    }

    void PawnRuntime::dispatchGuildMemberAdd(const std::string& guildId, const std::string& userId)
    {
        for (AMX* amx : scripts_)
        {
            if (amx == nullptr) continue;

            int index = -1;

            if (amx_FindPublic(amx, "DBridge_OnGuildMemberAdd", &index) != AMX_ERR_NONE) continue;

            cell guildAddress = 0;
            cell userAddress = 0;

            cell* guildPhysical = nullptr;
            cell* userPhysical = nullptr;

            if (amx_Allot(amx, static_cast<int>(guildId.size()) + 1, &guildAddress, &guildPhysical) != AMX_ERR_NONE) continue;

            if (amx_Allot(amx, static_cast<int>(userId.size()) + 1, &userAddress, &userPhysical) != AMX_ERR_NONE)
            {
                amx_Release(amx, guildAddress);
                continue;
            }

            amx_SetString(guildPhysical, guildId.c_str(), 0, 0, guildId.size() + 1);
            amx_SetString(userPhysical, userId.c_str(), 0, 0, userId.size() + 1);

            amx_Push(amx, userAddress);
            amx_Push(amx, guildAddress);

            cell returnValue = 0;

            amx_Exec(amx, &returnValue, index);

            amx_Release(amx, userAddress);
            amx_Release(amx, guildAddress);
        }
    }

    void PawnRuntime::dispatchGuildMemberRemove(const std::string& guildId, const std::string& userId)
    {
        for (AMX* amx : scripts_)
        {
            if (amx == nullptr) continue;

            int index = -1;

            if (amx_FindPublic(amx, "DBridge_OnGuildMemberRemove", &index) != AMX_ERR_NONE) continue;

            cell guildAddress = 0;
            cell userAddress = 0;

            cell* guildPhysical = nullptr;
            cell* userPhysical = nullptr;

            if (amx_Allot(amx, static_cast<int>(guildId.size()) + 1, &guildAddress, &guildPhysical) != AMX_ERR_NONE) continue;

            if (amx_Allot(amx, static_cast<int>(userId.size()) + 1, &userAddress, &userPhysical) != AMX_ERR_NONE)
            {
                amx_Release(amx, guildAddress);
                continue;
            }

            amx_SetString(guildPhysical, guildId.c_str(), 0, 0, guildId.size() + 1);
            amx_SetString(userPhysical, userId.c_str(), 0, 0, userId.size() + 1);

            amx_Push(amx, userAddress);
            amx_Push(amx, guildAddress);

            cell returnValue = 0;

            amx_Exec(amx, &returnValue, index);

            amx_Release(amx, userAddress);
            amx_Release(amx, guildAddress);
        }
    }

    std::size_t PawnRuntime::size() const
    {
        return scripts_.size();
    }
}