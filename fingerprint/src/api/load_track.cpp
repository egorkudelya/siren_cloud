#include "load_track.h"

namespace siren::cloud
{
    LoadTrackByUrlCallData::LoadTrackByUrlCallData(EnginePtr& engine, SirenFingerprint::AsyncService* service, const CompletionQueuePtr& completionQueue, WeakCollectorPtr collection)
        : CallData(engine, service, completionQueue, collection)
    {
        this->proceed();
    }

    void LoadTrackByUrlCallData::addNext()
    {
        if (auto sharedCollector = m_collector.lock())
        {
            sharedCollector->createNewCallData<LoadTrackByUrlCallData>(m_engine, m_service, m_completionQueue);
        }
        else
        {
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "sharedCollectorPtr has expired");
        }
    }

    void LoadTrackByUrlCallData::waitForRequest()
    {
        m_service->RequestLoadTrackByUrl(&m_serverContext, &m_request, &m_responder, m_completionQueue.get(), m_completionQueue.get(), this);
    }

    void LoadTrackByUrlCallData::handleRequest()
    {
        auto& req = getRequest();
        auto& reply = getReply();

        std::stringstream msg;
        msg << "Loading song with id " << req.song_id();
        Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, msg.str());

        reply.set_success(m_engine->loadTrackByUrl(req.url(), req.song_id(), req.is_caching()));
    }
}