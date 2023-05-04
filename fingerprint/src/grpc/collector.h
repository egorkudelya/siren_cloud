#pragma once
#include <unordered_set>
#include "calldata.h"

namespace siren::cloud
{
    class CallDataCollector: public std::enable_shared_from_this<CallDataCollector>
    {
    public:
        template<typename CallDataType, typename Service, typename CompletionQueue>
        void createNewCallData(EnginePtr& enginePtr, Service service, CompletionQueue queue)
        {
            CallDataPtr callDataPtr = std::make_shared<CallDataType>(enginePtr, service, queue, weak_from_this());
            std::lock_guard lock(m_mtx);
            m_registry.insert(std::move(callDataPtr));
        }

        bool requestCleanUpByThis(const CallDataBase* other);

    private:
        std::mutex m_mtx;
        std::unordered_set<CallDataPtr> m_registry;
    };

    using CollectorPtr = std::shared_ptr<CallDataCollector>;
    using WeakCollectorPtr = std::weak_ptr<CallDataCollector>;
}