#pragma once

#include "../grpc/collector.h"
#include "../grpc/server.h"
#include "fingerprint.grpc.pb.h"

namespace siren::cloud
{
    using fingerprint::SirenFingerprint;
    using fingerprint::DeleteTrackByIdRequest;
    using fingerprint::BasicIsSuccessResponse;

    class DeleteTrackByIdCallData: public CallData<SirenFingerprint, DeleteTrackByIdRequest, BasicIsSuccessResponse, WeakCollectorPtr>
    {
    public:
        DeleteTrackByIdCallData(EnginePtr& engine, SirenFingerprint::AsyncService* service, const CompletionQueuePtr& completionQueue, WeakCollectorPtr collection);

    private:
        void addNext() override;
        void waitForRequest() override;
        void handleRequest() override;
    };
}