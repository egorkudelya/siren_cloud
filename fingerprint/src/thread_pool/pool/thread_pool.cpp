#include "thread_pool.h"


ThreadPool::ThreadPool(size_t numberOfQueues, size_t threadsPerQueue, bool isGraceful)
    : m_is_graceful(isGraceful)
{
    std::unique_lock lock(m_pool_mtx);
    for (size_t i = 0; i < numberOfQueues; i++)
    {
        auto queue = std::make_shared<Queue>(i);
        m_primaryDispatch.insertQueue(queue);
        for (size_t j = 0; j < threadsPerQueue; j++)
        {
            std::thread th = std::thread(
            [this, queue]
            {
                 size_t id = std::hash<std::thread::id>{}(std::this_thread::get_id());
                 m_primaryDispatch.assignThreadIdToQueue(id, queue);
                 process();
            });
            m_threads.emplace_back(std::move(th));
        }
    }
    m_thread_count = numberOfQueues * threadsPerQueue;
    m_is_initialized = true;
}

void ThreadPool::process()
{
    size_t thisId = std::hash<std::thread::id>{}(std::this_thread::get_id());
    while (true)
    {
        if (m_is_pausing)
        {
            std::unique_lock lock(m_pause_mtx);
            m_pause_cv.wait(lock, [&] { return !m_is_pausing; });
        }
        else if (m_shut_down)
        {
            return;
        }
        Task task;
        if (m_is_initialized)
        {
            task = m_primaryDispatch.extractTaskByThreadId(thisId);
        }
        if (task.valid())
        {
            m_job_count--;
            m_running_job_count++;
            task();
            m_running_job_count--;
            m_wait_cv.notify_one();
        }
    }
}

void ThreadPool::waitForAll()
{
    std::unique_lock lock(m_wait_mtx);
    m_wait_cv.wait(lock, [&] { return m_job_count == 0 && m_running_job_count == 0; });
}

void ThreadPool::pause()
{
    m_is_pausing = true;
}

void ThreadPool::resume()
{
    m_is_pausing = false;
    m_pause_cv.notify_all();
}

size_t ThreadPool::getCurrentJobCount() const
{
    return m_job_count;
}

bool ThreadPool::areAllThreadsBusy() const
{
    return m_running_job_count == m_thread_count;
}

ThreadPool::~ThreadPool()
{
    std::unique_lock lock(m_pool_mtx);
    if (m_is_graceful)
    {
        waitForAll();
        release_assert(m_job_count == 0 && m_running_job_count == 0, "waitForAll didn't wait for all jobs to finish");
    }
    m_shut_down = true;
    m_primaryDispatch.clear();
    for (auto&& thread: m_threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
    m_is_initialized = false;
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

