#include "VersionNatives.hpp"
#include <Server/Components/Pawn/pawn.hpp>

namespace DiscordBridge
{
    static cell AMX_NATIVE_CALL Native_GetVersionMajor(AMX* amx, const cell* params)
    {
        (void)amx;
        (void)params;
        return GetVersionMajor();
    }

    static cell AMX_NATIVE_CALL Native_GetVersionMinor(AMX* amx, const cell* params)
    {
        (void)amx;
        (void)params;
        return GetVersionMinor();
    }

    static cell AMX_NATIVE_CALL Native_GetVersionPatch(AMX* amx, const cell* params)
    {
        (void)amx;
        (void)params;
        return GetVersionPatch();
    }

    int RegisterVersionNativesOpenMP(IPawnScript& script)
    {
        static AMX_NATIVE_INFO natives[] =
        {
            { "DBridge_GetVersionMajor", Native_GetVersionMajor },
            { "DBridge_GetVersionMinor", Native_GetVersionMinor },
            { "DBridge_GetVersionPatch", Native_GetVersionPatch },
            { nullptr, nullptr }
        };

        return script.Register(natives, -1);
    }
}