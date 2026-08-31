#pragma once

#include <amx/amx.h>

namespace DiscordBridge
{
    int GetVersionMajor();
    int GetVersionMinor();
    int GetVersionPatch();
    int RegisterNatives(AMX *amx);
}