#pragma once

#include "Embed.hpp"

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace DiscordBridge
{
    using EmbedHandle = std::uint32_t;

    class EmbedManager final
    {
    private:
        std::unordered_map<EmbedHandle, std::unique_ptr<Embed>> embeds_;
        EmbedHandle nextHandle_{1};

    public:
        EmbedManager() = default;
        ~EmbedManager() = default;

        EmbedManager(const EmbedManager&) = delete;
        EmbedManager& operator=(const EmbedManager&) = delete;

        EmbedHandle create();
        bool destroy(EmbedHandle handle);

        Embed* get(EmbedHandle handle);
        const Embed* get(EmbedHandle handle) const;

        bool exists(EmbedHandle handle) const;

        void clear();

        std::size_t size() const;
    };
}