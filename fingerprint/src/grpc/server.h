#pragma once

#include <future>
#include "collector.h"
#include "../engine/engine.h"

namespace siren::cloud
{
    template<typename Service, typename ...Args>
    class SirenServer
    {
        using AsyncService = typename Service::AsyncService;
    public:
        explicit SirenServer(const std::string& servAddress, EnginePtr& engine)
        : m_address(servAddress)
        , m_engine(engine)
        {
            m_collector = std::make_shared<CallDataCollector>();
        }

        ~SirenServer()
        {
            if (!m_done)
            {
                ShutDown();
            }
        }

        void Run()
        {
            std::string queueCountVar = siren::getenv("SERV_QUEUES");
            std::string threadsPerQueueVar = siren::getenv("SERV_THREADS_PER_QUEUE");

            size_t coreCount = std::thread::hardware_concurrency();
            size_t queueCount = !queueCountVar.empty() ? std::stoi(queueCountVar) : coreCount;
            size_t threadsPerQueue = !threadsPerQueueVar.empty() ? std::stoi(threadsPerQueueVar) : 1;

            ServerBuilder builder;

            builder.AddListeningPort(m_address, grpc::InsecureServerCredentials());
            builder.RegisterService(&m_service);
            InitCompletionQueues(queueCount, builder);

            m_server = builder.BuildAndStart();

            std::string logMessage = "Server listening on " + m_address;
            Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, logMessage);

            InitThreadPool(queueCount, threadsPerQueue);
        }

        void ShutDown()
        {
            Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, "Begin shutting down the server");
            m_server->Shutdown();
            m_server->Wait();

            Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, "Waiting for all active tasks to complete");
            std::unique_lock lock(m_mtx);
            m_cv.wait(lock, [this]{ return m_activeTaskCount == 0; });

            Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, "Shutting down completion queues");
            for (auto&& cq : m_completionQueues)
            {
                cq->Shutdown();
            }

            Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, "Shutting down thread pool");
            CloseThreadPool();
            m_done = true;

            Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, "The server has been shut down");
        }

    private:
        void InitCompletionQueues(size_t numberOfQueues, ServerBuilder& builder)
        {
            for (size_t i = 0; i < numberOfQueues; i++)
            {
                auto queue = builder.AddCompletionQueue();
                m_completionQueues.emplace_back(std::move(queue));
            }
        }

        void InitThreadPool(size_t numberOfQueues, size_t threadsPerQueue)
        {
            for (size_t i = 0; i < numberOfQueues; i++)
            {
                for (size_t j = 0; j < threadsPerQueue; j++)
                {
                    std::thread thread = std::thread([this, i]{HandleRpcs(m_completionQueues[i]);});
                    m_threads.emplace_back(std::move(thread));
                }
            }
        }

        void CloseThreadPool()
        {
            for (auto&& thread: m_threads)
            {
                if (thread.joinable())
                {
                    thread.join();
                }
            }
        }

        void HandleRpcs(CompletionQueuePtr& cq)
        {
            ([&]
             {
                m_collector->createNewCallData<Args>(m_engine, &m_service, cq);
             }
             (), ...);

            void* tag = nullptr;
            bool ok = false;
            while (cq->Next(&tag, &ok))
            {
                auto callData = static_cast<CallDataBase*>(tag);
                if (ok)
                {
                    m_activeTaskCount++;
                    callData->proceed();
                    m_activeTaskCount--;
                    m_cv.notify_one();
                }
                else
                {
                    Logger::log(LogLevel::WARNING, __FILE__, __FUNCTION__, __LINE__, "CompletionQueue is fully drained or shutting down");
                }
            }
        }

    private:
        std::string m_address;
        std::atomic<bool> m_done{false};
        std::atomic<int64_t> m_activeTaskCount{0};
        CollectorPtr m_collector;
        std::mutex m_mtx;
        std::condition_variable m_cv;
        std::vector<std::thread> m_threads;
        std::vector<CompletionQueuePtr> m_completionQueues;
        AsyncService m_service;
        std::unique_ptr<grpc::Server> m_server;
        EnginePtr m_engine;
    };

}