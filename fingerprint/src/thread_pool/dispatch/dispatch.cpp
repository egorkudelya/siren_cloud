#include "dispatch.h"

namespace siren::cloud
{

    QueueDispatch::const_iterator QueueDispatch::cbegin() const
    {
        std::shared_lock lock(m_set_mtx);
        return m_set.cbegin();
    }

    QueueDispatch::const_iterator QueueDispatch::cend() const
    {
        std::shared_lock lock(m_set_mtx);
        return m_set.cend();
    }

    QueueDispatch::iterator QueueDispatch::begin()
    {
        std::unique_lock lock(m_set_mtx);
        return m_set.begin();
    }

    QueueDispatch::iterator QueueDispatch::end()
    {
        std::unique_lock lock(m_set_mtx);
        return m_set.end();
    }

    size_t QueueDispatch::size() const
    {
        std::shared_lock lock(m_set_mtx);
        return m_set.size();
    }

    void QueueDispatch::assignThreadIdToQueue(size_t threadId, const QueuePtr& queuePtr)
    {
        std::unique_lock lock(m_map_mtx);
        m_map[threadId] = queuePtr;
    }

    Task QueueDispatch::extractTaskByThreadId(size_t threadId)
    {
        Map::iterator iter;
        {
            std::shared_lock lock(m_map_mtx);
            iter = m_map.find(threadId);
            if (iter == m_map.end())
            {
                Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, "Thread id not found in mapper");
            }
        }
        Task task;
        iter->second->pop(task);
        return task;
    }

    void QueueDispatch::pushTaskToLeastBusy(Task&& task)
    {
        QueuePtr queue = extractLeastBusy();
        queue->push(std::move(task));
        insertQueue(queue);
    }

    QueuePtr QueueDispatch::extractLeastBusy()
    {
        std::unique_lock lock(m_set_mtx);
        auto nodeHandle = m_set.extract(m_set.begin());
        return nodeHandle.value();
    }

    void QueueDispatch::insertQueue(const QueuePtr& queueBlockPtr)
    {
        std::unique_lock lock(m_set_mtx);
        m_set.insert(queueBlockPtr);
    }

    void QueueDispatch::clear()
    {
        std::unique_lock lock(m_map_mtx);
        for (const auto& bucket: m_map)
        {
            bucket.second->signalAbort();
        }
    }

    bool QueueDispatch::isPoolId(size_t id) const
    {
        std::shared_lock lock(m_map_mtx);
        auto it = m_map.find(id);
        if (it != m_map.end())
        {
            return true;
        }
        return false;
    }
}