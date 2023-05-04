#include "collector.h"

namespace siren::cloud
{
    bool CallDataCollector::requestCleanUpByThis(const CallDataBase* other)
    {
        std::lock_guard lock(m_mtx);
        auto it = std::find_if(m_registry.begin(), m_registry.end(), [&](const auto& ptr){return ptr.get() == other;});
        if (it != m_registry.end())
        {
            m_registry.erase(it);
            return true;
        }
        return false;
    }
}