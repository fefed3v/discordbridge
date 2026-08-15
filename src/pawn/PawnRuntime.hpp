#pragma once

#include <cstddef>
#include <vector>

#include <amx/amx.h>

namespace DiscordBridge
{
    class PawnRuntime final
    {
    private:
        std::vector<AMX*> scripts_;

    public:
        PawnRuntime() = default;
        ~PawnRuntime() = default;

        bool addAMX(AMX* amx);
        bool removeAMX(AMX* amx);

        void clear();
        void dispatchReady();

        std::size_t size() const;
    };
}