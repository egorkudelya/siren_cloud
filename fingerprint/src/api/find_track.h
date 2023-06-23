#pragma once
#include "../grpc/collector.h"
#include "../grpc/server.h"
#include "fingerprint.grpc.pb.h"

namespace siren::cloud
{
    using fingerprint::SirenFingerprint;
    using fingerprint::FindTrackByFingerprintRequest;
    using fingerprint::FindTrackByFingerprintResponse;

    class FindTrackByFingerprintCallData: public CallData<SirenFingerprint, FindTrackByFingerprintRequest, FindTrackByFingerprintResponse, WeakCollectorPtr>
    {
    public:
        FindTrackByFingerprintCallData(EnginePtr& engine, SirenFingerprint::AsyncService* service, const CompletionQueuePtr& completionQueue, WeakCollectorPtr collection);

    private:
        void addNext() override;
        void waitForRequest() override;
        void handleRequest() override;
    };
}