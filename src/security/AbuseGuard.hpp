#pragma once
#include <chrono>
#include <cstddef>
#include <mutex>
#include <string>
#include <unordered_map>

using namespace std;

namespace DiscordBridge
{
    class AbuseGuard
    {
    public:
        bool allow(const string &key, size_t limit, chrono::milliseconds window)
        {
            if (key.empty())
                return false;

            const auto now = chrono::steady_clock::now();
            lock_guard<mutex> lock(mutex_);
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
            lock_guard<mutex> lock(mutex_);
            entries_.clear();
        }

    private:
        struct Entry
        {
            chrono::steady_clock::time_point start{};
            size_t count{0};
        };
        mutex mutex_;
        unordered_map<string, Entry> entries_;
    };
}
