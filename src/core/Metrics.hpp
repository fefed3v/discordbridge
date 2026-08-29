#pragma once
#include <atomic>
#include <cstdint>
namespace DiscordBridge
{
    struct Metrics
    {
        std::atomic<std::uint64_t> droppedGatewayEvents{0};
        std::atomic<std::uint64_t> droppedRestRequests{0};
        std::atomic<std::uint64_t> droppedInteractionRequests{0};
        std::atomic<std::uint64_t> droppedResults{0};
        std::atomic<std::uint64_t> processedEvents{0};
        std::atomic<std::uint64_t> rateLimitedRequests{0};
    };
    inline Metrics &GlobalMetrics()
    {
        static Metrics metrics;
        return metrics;
    }
}
