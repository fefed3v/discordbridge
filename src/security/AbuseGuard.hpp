#pragma once
#include <chrono>
#include <cstddef>
#include <mutex>
#include <string>
#include <unordered_map>
namespace DiscordBridge
{
    class AbuseGuard
    {
    public:
        bool allow(const std::string &key, std::size_t limit, std::chrono::milliseconds window)
        {
            if (key.empty())
                return false;
            const auto now = std::chrono::steady_clock::now();
            std::lock_guard<std::mutex> lock(mutex_);
            auto &e = entries_[key];
            if (e.start.time_since_epoch().count() == 0 || now - e.start >= window)
            {
                e.start = now;
                e.count = 1;
                return true;
            }
            if (e.count >= limit)
                return false;
            ++e.count;
            return true;
        }
        void clear()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            entries_.clear();
        }

    private:
        struct Entry
        {
            std::chrono::steady_clock::time_point start{};
            std::size_t count{0};
        };
        std::mutex mutex_;
        std::unordered_map<std::string, Entry> entries_;
    };
}
