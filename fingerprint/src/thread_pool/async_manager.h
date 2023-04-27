#pragma once
#include "pool/thread_pool.h"

namespace siren::cloud
{
    class AsyncManager: public ThreadPoolProxy
    {

    public:
        AsyncManager(const AsyncManager& other) = delete;
        AsyncManager(AsyncManager&& other) = delete;
        AsyncManager& operator=(const AsyncManager& other) = delete;
        AsyncManager& operator=(AsyncManager&& other) = delete;

        static AsyncManager& instance()
        {
            static AsyncManager instance;
            return instance;
        }

    private:
        AsyncManager() = default;
    };
}