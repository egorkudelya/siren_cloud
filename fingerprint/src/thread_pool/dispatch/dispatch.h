#pragma once
#include <shared_mutex>
#include <unordered_map>
#include <set>
#include "../../common/safe_queue.h"
#include "../callback/callback.h"
#include "../primitives/task.h"
#include "../../common/common.h"

using Task = PackagedTask<void>;
using Queue = SafeQueue<Task>;
using QueuePtr = std::shared_ptr<Queue>;

class QueueDispatch
{
    using Set = std::set<QueuePtr, QueueCompare>;
    using iterator = Set::iterator;
    using const_iterator = Set::const_iterator;
    using Map = std::unordered_map<size_t, QueuePtr>;

public:
    QueueDispatch() = default;
    const_iterator cbegin() const;
    const_iterator cend() const;
    size_t size() const;
    bool isPoolId(size_t id) const;
    void insertQueue(const QueuePtr& queueBlockPtr);
    void assignThreadIdToQueue(size_t threadId, const QueuePtr& queuePtr);
    void pushTaskToLeastBusy(Task&& callBack);
    Task extractTaskByThreadId(size_t threadId);
    void clear();

private:
    iterator begin();
    iterator end();
    QueuePtr extractLeastBusy();

private:
    mutable std::shared_mutex m_set_mtx;
    mutable std::shared_mutex m_map_mtx;
    Set m_set;
    Map m_map;
};
