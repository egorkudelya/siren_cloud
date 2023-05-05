#pragma once
#include "../grpc/collector.h"
#include "../grpc/server.h"
#include "fingerprint.grpc.pb.h"

namespace siren::cloud
{
    using fingerprint::SirenFingerprint;
    using fingerprint::LoadTrackByUrlRequest;
    using fingerprint::BasicIsSuccessResponse;

    class LoadTrackByUrlCallData: public CallData<SirenFingerprint, LoadTrackByUrlRequest, BasicIsSuccessResponse, WeakCollectorPtr>
    {
    public:
        LoadTrackByUrlCallData(EnginePtr& engine, SirenFingerprint::AsyncService* service, const CompletionQueuePtr& completionQueue, WeakCollectorPtr collection);

    private:
        void addNext() override;
        void waitForRequest() override;
        void handleRequest() override;
    };
}