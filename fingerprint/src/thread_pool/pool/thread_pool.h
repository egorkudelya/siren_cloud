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
            if (!m_shutDown && m_isInitialized)
            {
                size_t thisId = std::hash<std::thread::id>{}(std::this_thread::get_id());
                if (m_primaryDispatch.isPoolId(thisId))
                {
                    return WaitableFuture{std::move(std::async(std::launch::async, invocable)), isWaiting};
                }
                Task task(CallBack(std::move(invocable), thisId));
                WaitableFuture future{std::move(task.getFuture()), isWaiting};
                m_jobCount++;
                m_primaryDispatch.pushTaskToLeastBusy(std::move(task));
                return future;
            }
            return {};
        }

    private:
        void waitForAll();
        void process();

    private:
        size_t m_threadCount;
        QueueDispatch m_primaryDispatch;
        std::vector<std::thread> m_threads;
        bool m_shutDown{false};
        bool m_isInitialized{false};
        bool m_isPausing{false};
        bool m_isGraceful{false};
        std::mutex m_pauseMtx;
        std::mutex m_waitMtx;
        std::mutex m_poolMtx;
        std::condition_variable m_waitCv;
        std::condition_variable m_pauseCv;
        std::atomic<int64_t> m_jobCount{0};
        std::atomic<int64_t> m_runningJobCount{0};
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
