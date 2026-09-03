#pragma once

#include <atomic>
#include <iostream>
#include <mutex>
#include <string>

namespace DiscordBridge
{
    class DebugLog
    {
    public:
        static void setEnabled(bool enabled) { enabled_.store(enabled, std::memory_order_relaxed); }
        static bool isEnabled() { return enabled_.load(std::memory_order_relaxed); }

        static void info(const std::string &area, const std::string &message)
        {
            if (!isEnabled()) return;
            write("DEBUG", area, message);
        }

        static void warn(const std::string &area, const std::string &message)
        {
            write("WARN", area, message);
        }

        static void error(const std::string &area, const std::string &message)
        {
            write("ERROR", area, message);
        }

    private:
        static void write(const char *level, const std::string &area, const std::string &message)
        {
            std::lock_guard<std::mutex> lock(outputMutex_);
            std::cout << "[DiscordBridge] [" << level << "] [" << area << "] " << message << std::endl;
        }

        inline static std::atomic_bool enabled_{false};
        inline static std::mutex outputMutex_;
    };
}
