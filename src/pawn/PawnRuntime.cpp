#include "PawnRuntime.hpp"

#include <algorithm>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

namespace DiscordBridge
{
    namespace
    {
        string Utf8ToAnsi(const string &value)
        {
#ifdef _WIN32
            if (value.empty())
                return {};
            const int wideSize = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
            if (wideSize <= 0)
                return value;
            wstring wide(static_cast<size_t>(wideSize), L'\0');
            if (MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), wide.data(), wideSize) <= 0)
                return value;
            const int ansiSize = WideCharToMultiByte(CP_ACP, 0, wide.data(), wideSize, nullptr, 0, nullptr, nullptr);
            if (ansiSize <= 0)
                return value;
            string result(static_cast<size_t>(ansiSize), '\0');
            if (WideCharToMultiByte(CP_ACP, 0, wide.data(), wideSize, result.data(), ansiSize, nullptr, nullptr) <= 0)
                return value;
            return result;
#else
            return value;
#endif
        }

        bool PushPawnString(AMX *amx, const string &value, cell &address)
        {
            cell *physical = nullptr;

            if (amx_Allot(amx, static_cast<int>(value.size()) + 1, &address, &physical) != AMX_ERR_NONE)
                return false;

            if (amx_SetString(physical, value.c_str(), 0, 0, value.size() + 1) == AMX_ERR_NONE)
                return true;

            amx_Release(amx, address);
            address = 0;

            return false;
        }

        bool FindPublic(AMX *amx, const char *name, int &index)
        {
            return amx && amx_FindPublic(amx, name, &index) == AMX_ERR_NONE;
        }

        void DispatchTwoStrings(const vector<AMX *> &scripts, const char *callback, const string &first, const string &second)
        {
            for (AMX *amx : scripts)
            {
                int index = -1;

                if (!FindPublic(amx, callback, index))
                    continue;

                cell firstAddress = 0;
                cell secondAddress = 0;

                if (!PushPawnString(amx, first, firstAddress))
                    continue;

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

        void DispatchThreeStrings(const vector<AMX *> &scripts, const char *callback, const string &first, const string &second, const string &third)
        {
            for (AMX *amx : scripts)
            {
                int index = -1;

                if (!FindPublic(amx, callback, index))
                    continue;

                cell firstAddress = 0;
                cell secondAddress = 0;
                cell thirdAddress = 0;

                if (!PushPawnString(amx, first, firstAddress))
                    continue;

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

        void DispatchStrings(const vector<AMX *> &scripts, const char *callback, const vector<string> &values)
        {
            for (AMX *amx : scripts)
            {
                int index = -1;
                if (!FindPublic(amx, callback, index))
                    continue;

                vector<cell> addresses;
                addresses.reserve(values.size());
                bool ok = true;

                for (const auto &value : values)
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
                    for (auto it = addresses.rbegin(); it != addresses.rend(); ++it)
                        amx_Push(amx, *it);
                    cell returnValue = 0;
                    amx_Exec(amx, &returnValue, index);
                }

                // AMX heap allocations are stack-like. They MUST be released in reverse order.
                for (auto it = addresses.rbegin(); it != addresses.rend(); ++it)
                    amx_Release(amx, *it);
            }
        }

        void DispatchResult(const vector<AMX *> &scripts, const char *callback, bool success, const string &channelId, const string &messageId)
        {
            for (AMX *amx : scripts)
            {
                int index = -1;

                if (!FindPublic(amx, callback, index))
                    continue;

                cell channelAddress = 0;
                cell messageAddress = 0;

                if (!PushPawnString(amx, channelId, channelAddress))
                    continue;

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

        void DispatchFetchOne(const vector<AMX *> &scripts, const char *callback, bool success, const string &id)
        {
            for (AMX *amx : scripts)
            {
                int index = -1;
                if (!FindPublic(amx, callback, index)) continue;
                cell address = 0;
                if (!PushPawnString(amx, id, address)) continue;
                amx_Push(amx, address);
                amx_Push(amx, success ? 1 : 0);
                cell returnValue = 0;
                amx_Exec(amx, &returnValue, index);
                amx_Release(amx, address);
            }
        }

        void DispatchFetchTwo(const vector<AMX *> &scripts, const char *callback, bool success, const string &first, const string &second)
        {
            for (AMX *amx : scripts)
            {
                int index = -1;
                if (!FindPublic(amx, callback, index)) continue;
                cell firstAddress = 0, secondAddress = 0;
                if (!PushPawnString(amx, first, firstAddress)) continue;
                if (!PushPawnString(amx, second, secondAddress)) { amx_Release(amx, firstAddress); continue; }
                amx_Push(amx, secondAddress);
                amx_Push(amx, firstAddress);
                amx_Push(amx, success ? 1 : 0);
                cell returnValue = 0;
                amx_Exec(amx, &returnValue, index);
                amx_Release(amx, secondAddress);
                amx_Release(amx, firstAddress);
            }
        }

        void DispatchDeployResult(const vector<AMX *> &scripts, bool success, const string &guildId)
        {
            for (AMX *amx : scripts)
            {
                int index = -1;
                if (!FindPublic(amx, "DBridge_OnCommandsDeployed", index)) continue;
                cell guildAddress = 0;
                if (!PushPawnString(amx, guildId, guildAddress)) continue;
                amx_Push(amx, guildAddress);
                amx_Push(amx, success ? 1 : 0);
                cell returnValue = 0;
                amx_Exec(amx, &returnValue, index);
                amx_Release(amx, guildAddress);
            }
        }
    }

    bool PawnRuntime::addAMX(AMX *amx)
    {
        if (!amx || find(scripts_.begin(), scripts_.end(), amx) != scripts_.end())
            return false;

        scripts_.push_back(amx);
        return true;
    }

    bool PawnRuntime::removeAMX(AMX *amx)
    {
        if (!amx)
            return false;

        const auto iterator = find(scripts_.begin(), scripts_.end(), amx);

        if (iterator == scripts_.end())
            return false;

        scripts_.erase(iterator);
        return true;
    }

    void PawnRuntime::clear()
    {
        scripts_.clear();
    }

    void PawnRuntime::dispatchReady()
    {
        for (AMX *amx : scripts_)
        {
            int index = -1;

            if (!FindPublic(amx, "DBridge_OnReady", index))
                continue;

            cell returnValue = 0;
            amx_Exec(amx, &returnValue, index);
        }
    }

    void PawnRuntime::dispatchMessageCreate(const string &userId, const string &channelId, const string &message)
    {
        DispatchThreeStrings(scripts_, "DBridge_OnMessageCreate", userId, channelId, Utf8ToAnsi(message));
    }

    void PawnRuntime::dispatchGuildMemberAdd(const string &guildId, const string &userId)
    {
        DispatchTwoStrings(scripts_, "DBridge_OnGuildMemberAdd", guildId, userId);
    }

    void PawnRuntime::dispatchGuildMemberRemove(const string &guildId, const string &userId)
    {
        DispatchTwoStrings(scripts_, "DBridge_OnGuildMemberRemove", guildId, userId);
    }

    void PawnRuntime::dispatchMessageSent(bool success, const string &channelId, const string &messageId)
    {
        DispatchResult(scripts_, "DBridge_OnMessageSent", success, channelId, messageId);
    }

    void PawnRuntime::dispatchMessageEdited(bool success, const string &channelId, const string &messageId)
    {
        DispatchResult(scripts_, "DBridge_OnMessageEdited", success, channelId, messageId);
    }

    void PawnRuntime::dispatchMessageDeleted(bool success, const string &channelId, const string &messageId)
    {
        DispatchResult(scripts_, "DBridge_OnMessageDeleted", success, channelId, messageId);
    }

    void PawnRuntime::dispatchEmbedSent(bool success, const string &channelId, const string &messageId)
    {
        DispatchResult(scripts_, "DBridge_OnEmbedSent", success, channelId, messageId);
    }

    void PawnRuntime::dispatchComponentsSent(bool success, const string &channelId, const string &messageId)
    {
        DispatchResult(scripts_, "DBridge_OnComponentsSent", success, channelId, messageId);
    }

    void PawnRuntime::dispatchV2Sent(bool success, const string &channelId, const string &messageId)
    {
        DispatchResult(scripts_, "DBridge_OnV2Sent", success, channelId, messageId);
    }

    void PawnRuntime::dispatchV2Edited(bool success, const string &channelId, const string &messageId)
    {
        DispatchResult(scripts_, "DBridge_OnV2Edited", success, channelId, messageId);
    }

    void PawnRuntime::dispatchChannelCreated(bool s, const string& g, const string& id) { DispatchResult(scripts_, "DBridge_OnChannelCreated", s, g, id); }
    void PawnRuntime::dispatchChannelDeleted(bool s, const string& g, const string& id) { DispatchResult(scripts_, "DBridge_OnChannelDeleted", s, g, id); }
    void PawnRuntime::dispatchRoleCreated(bool s, const string& g, const string& id) { DispatchResult(scripts_, "DBridge_OnRoleCreated", s, g, id); }
    void PawnRuntime::dispatchRoleDeleted(bool s, const string& g, const string& id) { DispatchResult(scripts_, "DBridge_OnRoleDeleted", s, g, id); }
    void PawnRuntime::dispatchMemberRoleAdded(bool s, const string& g, const string& id) { DispatchResult(scripts_, "DBridge_OnMemberRoleAdded", s, g, id); }
    void PawnRuntime::dispatchMemberRoleRemoved(bool s, const string& g, const string& id) { DispatchResult(scripts_, "DBridge_OnMemberRoleRemoved", s, g, id); }
    void PawnRuntime::dispatchMemberKicked(bool s, const string& g, const string& id) { DispatchResult(scripts_, "DBridge_OnMemberKicked", s, g, id); }
    void PawnRuntime::dispatchMemberBanned(bool s, const string& g, const string& id) { DispatchResult(scripts_, "DBridge_OnMemberBanned", s, g, id); }
    void PawnRuntime::dispatchMemberUnbanned(bool s, const string& g, const string& id) { DispatchResult(scripts_, "DBridge_OnMemberUnbanned", s, g, id); }
    void PawnRuntime::dispatchCommandsDeployed(bool s, const string& g) { DispatchDeployResult(scripts_, s, g); }
    void PawnRuntime::dispatchGuildFetched(bool s, const string& g) { DispatchFetchOne(scripts_, "DBridge_OnGuildFetched", s, g); }
    void PawnRuntime::dispatchChannelFetched(bool s, const string& c) { DispatchFetchOne(scripts_, "DBridge_OnChannelFetched", s, c); }
    void PawnRuntime::dispatchRoleFetched(bool s, const string& g, const string& r) { DispatchFetchTwo(scripts_, "DBridge_OnRoleFetched", s, g, r); }
    void PawnRuntime::dispatchMemberFetched(bool s, const string& g, const string& u) { DispatchFetchTwo(scripts_, "DBridge_OnMemberFetched", s, g, u); }
    void PawnRuntime::dispatchUserFetched(bool s, const string& u) { DispatchFetchOne(scripts_, "DBridge_OnUserFetched", s, u); }

    void PawnRuntime::dispatchButtonClick(const string &userId, const string &channelId, const string &customId, const string &interactionId, const string &interactionToken)
    {
        DispatchThreeStrings(scripts_, "DBridge_OnButtonClick", userId, channelId, customId);
        DispatchStrings(scripts_, "DBridge_OnButtonInteraction", {userId, channelId, customId, interactionId, interactionToken});
    }

    void PawnRuntime::dispatchSelectMenu(const string &userId, const string &channelId, const string &customId, const string &value, const string &interactionId, const string &interactionToken)
    {
        DispatchStrings(scripts_, "DBridge_OnSelectMenu", {userId, channelId, customId, value, interactionId, interactionToken});
    }

    void PawnRuntime::dispatchModalSubmit(const string &userId, const string &channelId, const string &customId, const vector<pair<string, string>> &values, const string &interactionId, const string &interactionToken)
    {
        activeModalValues_ = &values;
        DispatchStrings(scripts_, "DBridge_OnModalSubmit", {userId, channelId, customId, interactionId, interactionToken});
        activeModalValues_ = nullptr;
    }

    bool PawnRuntime::getModalValue(const string &customId, string &value) const
    {
        if (!activeModalValues_ || customId.empty())
            return false;
        for (const auto &entry : *activeModalValues_)
        {
            if (entry.first != customId)
                continue;
            value = entry.second;
            return true;
        }
        return false;
    }


    void PawnRuntime::dispatchSlashCommand(const string &commandName, const string &userId, const string &guildId, const string &channelId, const vector<pair<string, string>> &options, const string &interactionId, const string &interactionToken)
    {
        activeCommandValues_ = &options;
        DispatchStrings(scripts_, "DBridge_OnSlashCommand", {commandName, userId, guildId, channelId, interactionId, interactionToken});
        activeCommandValues_ = nullptr;
    }

    bool PawnRuntime::getCommandValue(const string &name, string &value) const
    {
        if (!activeCommandValues_ || name.empty()) return false;
        for (const auto &entry : *activeCommandValues_)
        {
            if (entry.first != name) continue;
            value = entry.second;
            return true;
        }
        return false;
    }

    size_t PawnRuntime::size() const
    {
        return scripts_.size();
    }
}