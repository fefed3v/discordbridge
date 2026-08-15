#include "EmbedManager.hpp"

namespace DiscordBridge
{
    EmbedHandle EmbedManager::create()
    {
        EmbedHandle handle = nextHandle_;

        while (handle == 0 || embeds_.find(handle) != embeds_.end())
        {
            ++handle;

            if (handle == 0) ++handle;
        }

        nextHandle_ = handle + 1;

        if (nextHandle_ == 0) nextHandle_ = 1;

        embeds_.emplace(handle, std::make_unique<Embed>());

        return handle;
    }

    bool EmbedManager::destroy(EmbedHandle handle)
    {
        if (handle == 0) return false;

        return embeds_.erase(handle) > 0;
    }

    Embed* EmbedManager::get(EmbedHandle handle)
    {
        const auto iterator = embeds_.find(handle);

        if (iterator == embeds_.end()) return nullptr;

        return iterator->second.get();
    }

    const Embed* EmbedManager::get(EmbedHandle handle) const
    {
        const auto iterator = embeds_.find(handle);

        if (iterator == embeds_.end()) return nullptr;

        return iterator->second.get();
    }

    bool EmbedManager::exists(EmbedHandle handle) const
    {
        return embeds_.find(handle) != embeds_.end();
    }

    void EmbedManager::clear()
    {
        embeds_.clear();
        nextHandle_ = 1;
    }

    std::size_t EmbedManager::size() const
    {
        return embeds_.size();
    }
}