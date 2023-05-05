#pragma once

#include <future>
#include <iostream>
#include <queue>
#include <mutex>

template <typename T>
class SafeQueue
{

public:
    SafeQueue(size_t id)
        :
          m_id(id)
        , m_size(0)
        , m_abort(false) {};

    void push(T&& task)
    {
        std::unique_lock lock(m_mtx);
        m_tasks.emplace(std::move(task));
        m_size++;
        m_cv.notify_one();
    }

    void push(const T& task)
    {
        std::unique_lock lock(m_mtx);
        m_tasks.push(task);
        m_size++;
        m_cv.notify_one();
    }

    bool pop(T& value)
    {
        {
            std::unique_lock lock(m_mtx);
            m_cv.wait(lock, [&] {return !m_tasks.empty() || m_abort;});

            if (m_abort)
            {
                return false;
            }

            value = std::move(m_tasks.front());
            m_tasks.pop();
            m_size--;
        }
        return true;
    }

    void signalAbort()
    {
        m_abort = true;
        m_cv.notify_all();
    }

    size_t getId() const
    {
        return m_id;
    }

    size_t getCurrentSize() const
    {
        return m_size;
    }

    bool isEmpty() const
    {
        return m_size == 0;
    }

private:
    size_t m_id;
    std::atomic<bool> m_abort;
    std::atomic<size_t> m_size;
    std::condition_variable m_cv;
    std::mutex m_mtx;
    std::queue<T> m_tasks;
};

template <typename T>
using SafeQueuePtr = std::shared_ptr<SafeQueue<T>>;

struct QueueCompare
{
    template <typename T>
    bool operator()(const SafeQueuePtr<T>& lhs, const SafeQueuePtr<T>& rhs) const
    {
        if (lhs->getCurrentSize() != rhs->getCurrentSize())
        {
            return lhs->getCurrentSize() < rhs->getCurrentSize();
        }
        return lhs->getId() < rhs->getId();
    }
};