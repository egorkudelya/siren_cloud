#include "delete_track.h"
#include "../thread_pool/async_manager.h"
#include "../common/request_manager.h"

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

        SongIdType songId = req.song_id();
        AsyncManager::instance().submitTask([this, songId] {
            if (!m_engine->purgeFingerprintBySongId(songId))
            {
                std::stringstream err;
                err << "Failed to delete fingerprint by song id " << songId;
                Logger::log(LogLevel::FATAL, __FILE__, __FUNCTION__, __LINE__, err.str());
                return;
            }

            std::string url = m_metadataAddr + "/api/records/do_delete/" + std::to_string(songId);
            HttpResponse metadataRes = RequestManager::Delete(url, {}, "Content-Type: application/json", {});

            if (metadataRes.status_code != 204)
            {
                std::stringstream err;
                err << "Failed to delete song metadata for song with id " << songId << ", Metadata returned code "
                    << metadataRes.status_code << " with error " << metadataRes.error.message;
                Logger::log(LogLevel::FATAL, __FILE__, __FUNCTION__, __LINE__, err.str());
            }
        });

        reply.set_success(true);
    }
}