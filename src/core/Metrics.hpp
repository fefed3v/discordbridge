#pragma once
#include <atomic>
#include <cstdint>

using namespace std;

namespace DiscordBridge
{
    struct Metrics
    {
        atomic<uint64_t> droppedGatewayEvents{0};
        atomic<uint64_t> droppedRestRequests{0};
        atomic<uint64_t> droppedInteractionRequests{0};
        atomic<uint64_t> droppedResults{0};
        atomic<uint64_t> processedEvents{0};
        atomic<uint64_t> rateLimitedRequests{0};
        atomic<uint64_t> reconnectAttempts{0};
        atomic<uint64_t> reconnectSuccesses{0};
        atomic<uint64_t> httpServerErrors{0};
        atomic<uint64_t> invalidPayloads{0};
    };
    inline Metrics &GlobalMetrics()
    {
        static Metrics metrics;
        return metrics;
    }
}
