#pragma once

struct tagAMX;
using AMX = tagAMX;

namespace DiscordBridge
{
    int GetVersionMajor();
    int GetVersionMinor();
    int GetVersionPatch();

    int RegisterVersionNatives(AMX* amx);
}