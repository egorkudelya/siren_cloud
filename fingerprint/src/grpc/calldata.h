#pragma once

#include <sstream>
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>
#include "../engine/engine.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;

namespace siren::cloud
{

    template<class T>
    struct is_weakPtr : std::false_type {};

    template<class T>
    struct is_weakPtr<std::weak_ptr<T>> : std::true_type {};

    enum class CallStatus
    {
        CREATE,
        PROCESS,
        FINISH
    };

    class CallDataBase
    {
    public:
        virtual ~CallDataBase() = default;
        CallDataBase(CallDataBase&& other) noexcept = default;
        CallDataBase& operator=(CallDataBase&& other) noexcept = default;

        virtual void proceed() = 0;
        virtual CallStatus getStatus() = 0;

    protected:
        virtual void waitForRequest() = 0;
        virtual void handleRequest() = 0;
    };

    using CallDataPtr = std::shared_ptr<CallDataBase>;
    using CompletionQueuePtr = std::shared_ptr<grpc::ServerCompletionQueue>;

    template<typename Service, typename RequestType, typename ReplyType, typename WeakCollectorPtr>
    class CallData: public CallDataBase
    {
        using AsyncService = typename Service::AsyncService;
        static_assert(is_weakPtr<WeakCollectorPtr>::value == true);

    public:
        CallData(EnginePtr& engine, AsyncService* service, const CompletionQueuePtr& completionQueue, WeakCollectorPtr collection)
            : m_status(CallStatus::CREATE)
            , m_service(service)
            , m_completionQueue(completionQueue)
            , m_responder(&m_serverContext)
            , m_engine(engine)
            , m_collector(collection)
        {
            m_metadataAddr = siren::getenv("METADATA_ADDRESS") + ':' + siren::getenv("METADATA_PORT");
        }

        virtual ~CallData() = default;
        CallData(CallData&& other) noexcept = default;
        CallData& operator=(CallData&& other) noexcept = default;

    public:
        void proceed() override
        {
            switch (m_status)
            {
                case CallStatus::CREATE:
                    m_status = CallStatus::PROCESS;
                    waitForRequest();
                    break;
                case CallStatus::PROCESS:
                    addNext();
                    handleRequest();
                    m_status = CallStatus::FINISH;
                    m_responder.Finish(m_reply, Status::OK, this);
                    break;
                case CallStatus::FINISH:
                {
                    auto sharedCollector = m_collector.lock();
                    if (!sharedCollector || !sharedCollector->requestCleanUpByThis(this))
                    {
                        Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Could not clean up calldata");
                    }
                    break;
                }
                default:
                    std::stringstream err;
                    err << "CallStatus " << (int)m_status << " is invalid";
                    Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, err.str());
            }
        }

        CallStatus getStatus() override
        {
            return m_status;
        }

        RequestType& getRequest()
        {
            return m_request;
        }

        ReplyType& getReply()
        {
            return m_reply;
        }

    protected:
        virtual void addNext() = 0;

    protected:
        AsyncService* m_service;
        WeakCollectorPtr m_collector;
        CallStatus m_status;
        CompletionQueuePtr m_completionQueue;
        RequestType m_request;
        ReplyType m_reply;
        ServerAsyncResponseWriter<ReplyType> m_responder;
        ServerContext m_serverContext;
        EnginePtr m_engine;
        std::string m_metadataAddr;
    };

} // namespace siren::service
