#include "find_track.h"
#include "../common/request_manager.h"

namespace siren::cloud
{
    namespace detail
    {
        static bool validateMetadataResponse(const grpc::protobuf::Descriptor* desc, const Json& target)
        {
            for (size_t i = 0; i < desc->field_count(); i++)
            {
                const grpc::protobuf::FieldDescriptor* field = desc->field(i);
                if (field->name() != "errors" && field->name() != "timestamp" && !target.contains(field->name()))
                {
                    return false;
                }
                switch (field->type())
                {
                    case grpc::protobuf::FieldDescriptor::TYPE_MESSAGE:
                        auto nestedDescriptor = field->message_type();
                        if (nestedDescriptor->name() == "Error")
                        {
                            return true;
                        }

                        if (field->is_repeated())
                        {
                            for (auto&& json: target[field->name()])
                            {
                                if (!validateMetadataResponse(nestedDescriptor, json))
                                {
                                    return false;
                                }
                            }
                        }
                        else
                        {
                            const auto& body = target[field->name()];
                            if (!body.is_null() && !validateMetadataResponse(nestedDescriptor, body))
                            {
                                return false;
                            }
                        }
                        break;
                }
            }
            return true;
        }
    }// namespace detail

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
        auto engineRes = m_engine->findSongIdByFingerprint(isSuccess, std::move(fingerprint));
        if (engineRes.getStatus() != HistStatus::OK)
        {
            auto error = reply.add_errors();
            error->set_message(R"({"errors":{"detail":"Not Found"}})");
            Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, "Could not find a song by provided fingerprint");
            return;
        }

        std::string url = m_metadataAddr + "/api/records/" + std::to_string(engineRes.getSongId());
        HttpResponse metadataRes = RequestManager::Get(url, {}, "Content-Type: application/json", {});

        if (metadataRes.status_code != 200)
        {
            auto error = reply.add_errors();
            error->set_message(R"({"errors":{"detail":"Internal server error"}})");
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Failed to obtain song metadata");
            return;
        }

        Json metadataJson = Json::parse(metadataRes.text)["data"];
        if (metadataJson.empty() || !detail::validateMetadataResponse(fingerprint::FindTrackByFingerprintResponse::GetDescriptor(), metadataJson))
        {
            auto error = reply.add_errors();
            error->set_message(R"({"errors":{"detail":"Internal server error"}})");
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Metadata serv returned an invalid json response");
            return;
        }

        reply.set_name(metadataJson["name"]);
        reply.set_art_url(metadataJson["art_url"]);
        reply.set_audio_url(metadataJson["audio_url"]);
        reply.set_bit_rate(metadataJson["bit_rate"]);
        reply.set_duration(metadataJson["duration"]);
        reply.set_date_recorded(metadataJson["date_recorded"]);
        reply.set_timestamp(engineRes.getTimestamp());

        if (!metadataJson["single"].is_null())
        {
            auto single = new fingerprint::AlbumResponse();
            single->set_name(metadataJson["single"]["name"]);
            single->set_art_url(metadataJson["single"]["art_url"]);
            single->set_is_single(metadataJson["single"]["is_single"]);
            reply.set_allocated_single(single);
        }

        for (auto&& artistJson: metadataJson["artists"])
        {
            auto artist = reply.add_artists();
            artist->set_name(artistJson["name"]);
        }

        for (auto&& albumJson: metadataJson["albums"])
        {
            auto album = reply.add_albums();
            album->set_name(albumJson["name"]);
            album->set_art_url(albumJson["art_url"]);
            album->set_is_single(albumJson["is_single"]);
        }

        for (auto&& genreJson: metadataJson["genres"])
        {
            auto genre = reply.add_genres();
            genre->set_name(genreJson["name"]);
        }
    }
}