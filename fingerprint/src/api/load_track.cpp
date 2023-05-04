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

        SongIdType songId = req.song_id();
        std::string url = req.url();
        bool isCaching = req.is_caching();

        bool isSuccess = m_engine->loadTrackByUrl(url, songId, isCaching);
        reply.set_success(isSuccess);
    }
}