#include "VersionNatives.hpp"
#include "../../main/Version.hpp"

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
}