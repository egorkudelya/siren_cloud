#include "find_track.h"
#include "../common/request_manager.h"

namespace siren::cloud
{
    FindTrackByFingerprintCallData::FindTrackByFingerprintCallData(EnginePtr& engine, SirenFingerprint::AsyncService* service, const CompletionQueuePtr& completionQueue, WeakCollectorPtr collection)
        : CallData(engine, service, completionQueue, collection)
    {
        m_metadataAddr = siren::getenv("METADATA_ADDRESS") + ':' + siren::getenv("METADATA_PORT") + "/api/records/";
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
        auto engineRes = m_engine->findSongIdByFingerprint(isSuccess, std::move(fingerprint));
        if (engineRes.getStatus() != HistStatus::OK)
        {
            Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, "Could not find song by provided fingerprint");
            reply.set_data(R"({"errors":{"detail":"Not Found"}})");
            return;
        }

        std::string url = m_metadataAddr + std::to_string(engineRes.getSongId());
        HttpResponse metadataRes = RequestManager::Get(url, {}, "Content-Type: application/json", {});
        if (metadataRes.status_code == 200)
        {
            reply.set_data(metadataRes.text);
            return;
        }
        Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, "Failed to obtain a response from Fingerprint service");
        reply.set_data(R"({"errors":{"detail":"Internal server error"}})");
    }
}