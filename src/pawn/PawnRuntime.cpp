#include "PawnRuntime.hpp"

#include <algorithm>

namespace DiscordBridge
{
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

    std::size_t PawnRuntime::size() const
    {
        return scripts_.size();
    }
}