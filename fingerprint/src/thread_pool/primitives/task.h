#pragma once
#include <future>
#include "../callback/callback.h"

namespace siren::cloud
{
    template<typename T>
    class PackagedTask
    {
    public:
        PackagedTask() = default;
        PackagedTask(CallBack&& callback)
        {
            m_id = callback.getSenderId();
            std::packaged_task<T()> task(std::move(callback));
            m_task = std::move(task);
        }

        T operator()()
        {
            return m_task();
        }

        auto getFuture()
        {
            return m_task.get_future();
        }

        size_t getId() const
        {
            return m_id;
        }

        bool valid() const
        {
            return m_task.valid();
        }

    private:
        size_t m_id{0};
        std::packaged_task<T()> m_task;
    };
}