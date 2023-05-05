#pragma once

#include <siren_core/src/siren.h>
#include "../histogram/histogram.h"
#include "../storage/connection_pool.h"

namespace siren_core
{
    siren::SirenCore* CreateCore();
}

namespace siren::cloud
{
    using SirenCorePtr = std::shared_ptr<siren::SirenCore>;

    struct EngineParameters
    {
        EngineParameters();

        std::string postgresConnString;
        std::string elasticConnString;
    };

    class Engine
    {
    public:
        explicit Engine(const DBConnectionPoolPtr& primaryPool, const DBConnectionPoolPtr& cachePool, const SirenCorePtr& corePtr);
        ~Engine();
        HistReturnType findSongIdByFingerprint(bool& isSuccess, FingerprintType&& fingerprint);
        bool loadTrackByUrl(const std::string& url, SongIdType songId, bool isCaching=true);
        bool purgeFingerprintBySongId(SongIdType songId);

    private:
        bool isSongIdValid(SongIdType songId);
        bool isSongIdInCache(bool& exists, SongIdType songId);
        bool isSongIdInPrimary(bool& exists, SongIdType songId);
        bool cacheFingerprintBySongId(SongIdType songId);
        bool loadFingerprintIntoPrimary(const FingerprintType& fingerprint, SongIdType songId);
        bool loadFingerprintIntoCache(const FingerprintType& fingerprint, SongIdType songId);
        bool purgeTrackFingerprintFromPrimary(SongIdType songId);
        bool purgeTrackFingerprintFromCache(SongIdType songId);
        DBCommandPtr fetchFingerprintsFromCache(bool& isSuccess, const FingerprintType& snippet);
        DBCommandPtr fetchFingerprintsFromPrimary(bool& isSuccess, const FingerprintType& snippet);
        void markAsyncStart();
        void markAsyncEnd();

    private:
        std::mutex m_mtx;
        std::condition_variable m_cv;
        std::atomic<int64_t> m_asyncCount{0};
        SirenCorePtr m_sirenCore;
        DBConnectionPoolPtr m_primaryPool;
        DBConnectionPoolPtr m_cachePool;
    };

    using EnginePtr = std::shared_ptr<Engine>;
}