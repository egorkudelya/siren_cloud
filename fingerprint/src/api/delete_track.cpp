#include "delete_track.h"

namespace siren::cloud
{
    DeleteTrackByIdCallData::DeleteTrackByIdCallData(EnginePtr& engine, SirenFingerprint::AsyncService* service, const CompletionQueuePtr& completionQueue, WeakCollectorPtr collection)
        : CallData(engine, service, completionQueue, collection)
    {
        this->proceed();
    }

    void DeleteTrackByIdCallData::addNext()
    {
        if (auto sharedCollector = m_collector.lock())
        {
            sharedCollector->createNewCallData<DeleteTrackByIdCallData>(m_engine, m_service, m_completionQueue);
        }
        else
        {
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "sharedCollectorPtr has expired");
        }
    }

    void DeleteTrackByIdCallData::waitForRequest()
    {
        m_service->RequestDeleteTrackById(&m_serverContext, &m_request, &m_responder, m_completionQueue.get(), m_completionQueue.get(), this);
    }

    void DeleteTrackByIdCallData::handleRequest()
    {
        auto& req = getRequest();
        auto& reply = getReply();

        std::stringstream msg;
        msg << "Deleting fingerprint of song with id " << req.song_id();
        Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, msg.str());

        reply.set_success(m_engine->purgeFingerprintBySongId(req.song_id()));
    }
}