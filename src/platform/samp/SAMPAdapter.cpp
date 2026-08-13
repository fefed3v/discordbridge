#include "SAMPAdapter.hpp"

namespace DiscordBridge
{
    ServerPlatform SAMPAdapter::getPlatform() const
    {
        return ServerPlatform::SAMP;
    }

    bool SAMPAdapter::initialize()
    {
        if (initialized_)
        {
            return true;
        }

        initialized_ = true;

        return true;
    }

    void SAMPAdapter::shutdown()
    {
        if (!initialized_)
        {
            return;
        }

        initialized_ = false;
    }

    void SAMPAdapter::process()
    {
        if (!initialized_)
        {
            return;
        }
    }
}