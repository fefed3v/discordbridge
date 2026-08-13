#pragma once

#include <cstddef>
#include <mutex>
#include <vector>

struct tagAMX;
using AMX = tagAMX;

namespace DiscordBridge
{
    class PawnRuntime final
    {
    public:
        PawnRuntime() = default;
        ~PawnRuntime() = default;

        PawnRuntime(const PawnRuntime&) = delete;
        PawnRuntime& operator=(const PawnRuntime&) = delete;

        PawnRuntime(PawnRuntime&&) = delete;
        PawnRuntime& operator=(PawnRuntime&&) = delete;

        bool addAMX(AMX* amx);
        bool removeAMX(AMX* amx);

        bool containsAMX(AMX* amx) const;

        std::size_t getAMXCount() const;

        void clear();

    private:
        mutable std::mutex mutex_;
        std::vector<AMX*> amxList_;
    };
}