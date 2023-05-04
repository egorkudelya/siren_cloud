#include "find_track.h"

namespace siren::cloud
{
    FindTrackByFingerprintCallData::FindTrackByFingerprintCallData(EnginePtr& engine, SirenFingerprint::AsyncService* service, const CompletionQueuePtr& completionQueue, WeakCollectorPtr collection)
        : CallData(engine, service, completionQueue, collection)
    {
        this->proceed();
    }

    void FindTrackByFingerprintCallData::addNext()
    {
        if (auto sharedCollector = m_collector.lock())
        {
            sharedCollector->createNewCallData<FindTrackByFingerprintCallData>(m_engine, m_service, m_completionQueue);
        }
        else
        {
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "sharedCollectorPtr has expired");
        }
    }

    void FindTrackByFingerprintCallData::waitForRequest()
    {
        m_service->RequestFindTrackByFingerprint(&m_serverContext, &m_request, &m_responder, m_completionQueue.get(), m_completionQueue.get(), this);
    }

    void FindTrackByFingerprintCallData::handleRequest()
    {
        auto& req = getRequest();
        auto& reply = getReply();

        auto& map = req.fingerprint();
        FingerprintType fingerprint(map.begin(), map.end());
        bool isSuccess = false;
        auto res = m_engine->findSongIdByFingerprint(isSuccess, std::move(fingerprint));

        // TODO
        reply.set_song_metadata("");
    }
}