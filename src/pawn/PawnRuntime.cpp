#include "PawnRuntime.hpp"

#include <algorithm>

namespace DiscordBridge
{
    bool PawnRuntime::addAMX(AMX* amx)
    {
        if (amx == nullptr) return false;

        std::lock_guard<std::mutex> lock(mutex_);
        if (std::find(amxList_.begin(), amxList_.end(), amx) != amxList_.end()) return false;

        amxList_.push_back(amx);
        return true;
    }

    bool PawnRuntime::removeAMX(AMX* amx)
    {
        if (amx == nullptr) return false;

        std::lock_guard<std::mutex> lock(mutex_);
        const auto iterator = std::find(amxList_.begin(), amxList_.end(), amx);
        if (iterator == amxList_.end()) return false;

        amxList_.erase(iterator);
        return true;
    }

    bool PawnRuntime::containsAMX(AMX* amx) const
    {
        if (amx == nullptr) return false;

        std::lock_guard<std::mutex> lock(mutex_);
        return std::find(amxList_.begin(), amxList_.end(), amx) != amxList_.end();
    }

    std::size_t PawnRuntime::getAMXCount() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return amxList_.size();
    }

    void PawnRuntime::clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        amxList_.clear();
    }
}
