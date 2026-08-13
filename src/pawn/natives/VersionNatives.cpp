#include "VersionNatives.hpp"

#include "../../main/Version.hpp"

#include <amx/amx.h>

namespace DiscordBridge
{
    int GetVersionMajor()
    {
        return VERSION_MAJOR;
    }

    int GetVersionMinor()
    {
        return VERSION_MINOR;
    }

    int GetVersionPatch()
    {
        return VERSION_PATCH;
    }

    static cell AMX_NATIVE_CALL Native_GetVersionMajor(AMX* amx, cell* params)
    {
        (void)amx;
        (void)params;
        return GetVersionMajor();
    }

    static cell AMX_NATIVE_CALL Native_GetVersionMinor(AMX* amx, cell* params)
    {
        (void)amx;
        (void)params;
        return GetVersionMinor();
    }

    static cell AMX_NATIVE_CALL Native_GetVersionPatch(AMX* amx, cell* params)
    {
        (void)amx;
        (void)params;
        return GetVersionPatch();
    }

    int RegisterVersionNatives(AMX* amx)
    {
        if (amx == nullptr) return AMX_ERR_PARAMS;

        static AMX_NATIVE_INFO natives[] =
        {
            { "DBridge_GetVersionMajor", Native_GetVersionMajor },
            { "DBridge_GetVersionMinor", Native_GetVersionMinor },
            { "DBridge_GetVersionPatch", Native_GetVersionPatch },
            { nullptr, nullptr }
        };

        return amx_Register(amx, natives, -1);
    }
}