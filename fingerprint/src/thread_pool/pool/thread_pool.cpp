#include "thread_pool.h"

namespace siren::cloud
{

    ThreadPool::ThreadPool(size_t numberOfQueues, size_t threadsPerQueue, bool isGraceful)
        : m_isGraceful(isGraceful)
    {
        std::unique_lock lock(m_poolMtx);
        for (size_t i = 0; i < numberOfQueues; i++)
        {
            auto queue = std::make_shared<Queue>(i);
            m_primaryDispatch.insertQueue(queue);
            for (size_t j = 0; j < threadsPerQueue; j++)
            {
                std::thread thread = std::thread(
                    [this, queue] {
                        size_t id = std::hash<std::thread::id>{}(std::this_thread::get_id());
                        m_primaryDispatch.assignThreadIdToQueue(id, queue);
                        process();
                    });
                m_threads.emplace_back(std::move(thread));
            }
        }
        m_threadCount = numberOfQueues * threadsPerQueue;
        m_isInitialized = true;
        Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, "ThreadPool has been initialized");
    }

    void ThreadPool::process()
    {
        size_t thisId = std::hash<std::thread::id>{}(std::this_thread::get_id());
        while (true)
        {
            if (m_isPausing)
            {
                std::unique_lock lock(m_pauseMtx);
                m_pauseCv.wait(lock, [&] { return !m_isPausing; });
            }
            else if (m_shutDown)
            {
                return;
            }
            Task task;
            if (m_isInitialized)
            {
                task = m_primaryDispatch.extractTaskByThreadId(thisId);
            }
            if (task.valid())
            {
                m_jobCount--;
                m_runningJobCount++;
                task();
                m_runningJobCount--;
                m_waitCv.notify_one();
            }
        }
    }

    void ThreadPool::waitForAll()
    {
        std::unique_lock lock(m_waitMtx);
        m_waitCv.wait(lock, [&] { return m_jobCount == 0 && m_runningJobCount == 0; });
    }

    void ThreadPool::pause()
    {
        m_isPausing = true;
    }

    void ThreadPool::resume()
    {
        m_isPausing = false;
        m_pauseCv.notify_all();
    }

    size_t ThreadPool::getCurrentJobCount() const
    {
        return m_jobCount;
    }

    bool ThreadPool::areAllThreadsBusy() const
    {
        return m_runningJobCount == m_threadCount;
    }

    ThreadPool::~ThreadPool()
    {
        std::unique_lock lock(m_poolMtx);
        if (m_isGraceful)
        {
            waitForAll();
            if (m_jobCount != 0 || m_runningJobCount != 0)
            {
                Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "waitForAll didn't wait for all jobs to finish execution");
            }
        }
        m_shutDown = true;
        m_primaryDispatch.clear();
        for (auto&& thread: m_threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
        m_isInitialized = false;
        Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, "ThreadPool has been shut down");
    }

    ThreadPoolProxy::ThreadPoolProxy()
    {
        size_t coreCount = std::thread::hardware_concurrency();

        std::string queueCountStr = siren::getenv("THREAD_POOL_QUEUES");
        size_t queueCount = !queueCountStr.empty() ? std::stoul(queueCountStr) : coreCount;

        std::string threadCountStr = siren::getenv("TP_THREADS_PER_QUEUE");
        size_t threadsPerQueue = !threadCountStr.empty() ? std::stoul(threadCountStr) : coreCount;

        m_pool = std::make_shared<ThreadPool>(queueCount, threadsPerQueue, true);
    }
}