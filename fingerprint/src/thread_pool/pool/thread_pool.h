#pragma once
#include <thread>
#include <vector>
#include <condition_variable>
#include "../dispatch/dispatch.h"
#include "../primitives/waitable_future.h"
#include "../../common/common.h"

namespace siren::cloud
{
    class ThreadPool
    {
    public:
        ThreadPool(size_t numberOfQueues=3, size_t threadsPerQueue=2, bool isGraceful=true);
        ~ThreadPool();

        ThreadPool() = delete;
        ThreadPool(const ThreadPool& other) = delete;
        ThreadPool& operator=(ThreadPool&& other) = delete;
        ThreadPool(ThreadPool&& other) noexcept = delete;
        ThreadPool& operator=(const ThreadPool& other) noexcept = delete;

        void pause();
        void resume();
        size_t getCurrentJobCount() const;
        bool areAllThreadsBusy() const;

        template <typename Invocable>
        WaitableFuture submitTask(Invocable&& invocable, bool isWaiting=false)
        {
            if (!m_shut_down && m_is_initialized)
            {
                size_t thisId = std::hash<std::thread::id>{}(std::this_thread::get_id());
                if (m_primaryDispatch.isPoolId(thisId))
                {
                    return WaitableFuture{std::move(std::async(std::launch::async, invocable)), isWaiting};
                }
                Task task(CallBack(std::move(invocable), thisId));
                WaitableFuture future{std::move(task.getFuture()), isWaiting};
                m_job_count++;
                m_primaryDispatch.pushTaskToLeastBusy(std::move(task));
                return future;
            }
            return {};
        }

    private:
        void waitForAll();
        void process();

    private:
        size_t m_thread_count;
        QueueDispatch m_primaryDispatch;
        std::vector<std::thread> m_threads;
        bool m_shut_down{false};
        bool m_is_initialized{false};
        bool m_is_pausing{false};
        bool m_is_graceful{false};
        std::mutex m_pause_mtx;
        std::mutex m_wait_mtx;
        std::mutex m_pool_mtx;
        std::condition_variable m_wait_cv;
        std::condition_variable m_pause_cv;
        std::atomic<int64_t> m_job_count{0};
        std::atomic<int64_t> m_running_job_count{0};
    };

    using ThreadPoolPtr = std::shared_ptr<ThreadPool>;

    class ThreadPoolProxy
    {
    public:
        ThreadPoolProxy();

        template<typename Invocable>
        WaitableFuture submitTask(Invocable&& task, bool isWaiting=false)
        {
            static_assert(std::is_invocable_v<Invocable>, "submitTask accept only invocable objects");
            return m_pool->submitTask(std::forward<Invocable>(task), isWaiting);
        }

    private:
        ThreadPoolPtr m_pool;
    };
}
