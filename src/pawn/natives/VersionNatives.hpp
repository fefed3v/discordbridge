#pragma once

struct tagAMX;
using AMX = tagAMX;

class IPawnScript;

namespace DiscordBridge
{
    int GetVersionMajor();
    int GetVersionMinor();
    int GetVersionPatch();

    int RegisterVersionNativesOpenMP(IPawnScript& script);
    int RegisterVersionNativesSAMP(AMX* amx);
}