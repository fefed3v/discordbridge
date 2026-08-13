#include "OpenMPAdapter.hpp"

namespace DiscordBridge
{
    ServerPlatform OpenMPAdapter::getPlatform() const
    {
        return ServerPlatform::OpenMP;
    }

    bool OpenMPAdapter::initialize()
    {
        if (initialized_)
        {
            return true;
        }

        initialized_ = true;

        return true;
    }

    void OpenMPAdapter::shutdown()
    {
        if (!initialized_)
        {
            return;
        }

        initialized_ = false;
    }

    void OpenMPAdapter::process()
    {
        if (!initialized_)
        {
            return;
        }
    }
}