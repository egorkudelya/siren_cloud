#include "engine.h"
#include "../thread_pool/async_manager.h"
#include "../common/request_manager.h"
#include "../common/common.h"
#include <filesystem>
#include <regex>

namespace siren_core
{
    siren::SirenCore* CreateCore()
    {
        siren::CoreSpecification spec;

        std::string zscore = siren::getenv("CORE_PEAK_ZSCORE");
        std::string block_size = siren::getenv("CORE_BLOCK_SIZE");
        std::string stride_coeff = siren::getenv("CORE_BLOCK_STRIDE_COEFF");

        if (!zscore.empty())
        {
        #ifndef __clang__
            siren::convert_to_type(zscore, spec.core_params.target_zscore);
        #else
            float zscore_f = std::stof(zscore);
            release_assert(!isnan(zscore_f), "zscore_f is nan");
            spec.core_params.target_zscore = zscore_f;
        #endif
        }
        if (!block_size.empty())
        {
            siren::convert_to_type(block_size, spec.core_params.target_block_size);
        }
        if (!stride_coeff.empty())
        {
        #ifndef __clang__
            siren::convert_to_type(stride_coeff, spec.core_params.stride_coeff);
        #else
            float stride_coeff_f = std::stof(stride_coeff);
            release_assert(!isnan(stride_coeff_f), "stride_coeff_f is nan");
            spec.core_params.stride_coeff = stride_coeff_f;
        #endif
        }
        return new siren::SirenCore(std::move(spec));
    }
}

namespace siren::cloud
{
    EngineParameters::EngineParameters()
    {
        std::stringstream postgres, elastic;

        postgres << "postgresql://" << siren::getenv("POSTGRES_USER") << ':' << siren::getenv("POSTGRES_PASSWORD")
                 << '@' << siren::getenv("POSTGRES_HOST") << ':' << siren::getenv("POSTGRES_PORT")
                 << '/' << siren::getenv("POSTGRES_DB_NAME");
        postgresConnString = postgres.str();

        elastic << "https://" << siren::getenv("ELASTIC_HOST") << ":" << siren::getenv("ES_PORT") << "/";
        elasticConnString = elastic.str();
    }

    Engine::Engine(const DBConnectionPoolPtr& primaryPool, const DBConnectionPoolPtr& cachePool, const SirenCorePtr& corePtr)
       : m_sirenCore(corePtr)
       , m_primaryPool(primaryPool)
       , m_cachePool(cachePool)
    {
    }

    Engine::~Engine()
    {
        std::unique_lock lock(m_mtx);
        m_cv.wait(lock, [&]{ return m_asyncCount == 0; });
    }

    void Engine::markAsyncStart()
    {
        m_asyncCount++;
    }

    void Engine::markAsyncEnd()
    {
        m_asyncCount--;
        m_cv.notify_one();
    }

    bool Engine::cacheFingerprintBySongId(SongIdType songId)
    {
        bool exists;
        bool isOk = isSongIdInCache(exists, songId);

        if (!isOk)
        {
            return false;
        }
        if (exists)
        {
            Logger::log(LogLevel::WARNING, __FILE__, __FUNCTION__, __LINE__, "Tried to cache a song already in cache");
            return false;
        }

        DBConnectionPtr postgresConnection = m_primaryPool->getConnection();
        Query postgresReq;
        std::string sql = "SELECT hash, timestamp, song_id FROM fingerprint WHERE song_id = " + std::to_string(songId);
        postgresReq.emplace("query", sql);

        DBCommandPtr postgresCommand = postgresConnection->createCommand(std::move(postgresReq));
        if (!postgresCommand->execute() || postgresCommand->isEmpty())
        {
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Could not fetch data from primary storage for caching");
            m_primaryPool->releaseConnection(std::move(postgresConnection));
            return false;
        }

        QueryCollection elasticQueries;
        elasticQueries.reserve(postgresCommand->getSize());

        std::stringstream ss;
        while (postgresCommand->fetchNext())
        {
            Query elasticQuery;
            elasticQuery.emplace("lucene", "fingerprint/_bulk");
            elasticQuery.emplace("request_type", "POST");
            elasticQuery.emplace("header", "{\"index\": {}}");

            HashType hash;
            TimestampType ts;
            postgresCommand->asUint64("hash", hash);
            postgresCommand->asInt32("timestamp", ts);

            ss << "{\"hash\": " << hash << ',';
            ss << "\"song_id\": " << songId << ',';
            ss << "\"timestamp\": " << ts << '}';

            elasticQuery.emplace("query", ss.str());
            elasticQueries.insertQuery(std::move(elasticQuery));

            ss.clear();
            ss.str({});
        }

        DBConnectionPtr elasticConnection = m_cachePool->getConnection();
        DBCommandPtr elasticCommand = elasticConnection->createCommand(std::move(elasticQueries));

        bool isSuccess = elasticCommand->execute();

        m_cachePool->releaseConnection(std::move(elasticConnection));
        m_primaryPool->releaseConnection(std::move(postgresConnection));

        return isSuccess;
    }

    DBCommandPtr Engine::fetchFingerprintsFromCache(bool& isSuccess, const FingerprintType& snippet)
    {
        std::string batchSize = siren::getenv("ELASTIC_BATCH_SIZE");
        size_t optimalBatchSize = !batchSize.empty() ? std::stoul(batchSize) : 500;

        std::string windowSize = siren::getenv("ES_RESULT_WINDOW");
        size_t optimalWindowSize = !windowSize.empty() ? std::stoul(windowSize) : 500;

        std::string focusBuckets = siren::getenv("ES_FOCUS_BUCKETS");
        size_t focusBucketsCount = !focusBuckets.empty() ? std::stoul(focusBuckets) : 35;

        size_t shardSize = focusBucketsCount * optimalWindowSize;
        auto formQuery = [shardSize, optimalWindowSize, focusBucketsCount](std::string&& hashes)
        {
            std::stringstream ss;
            ss << R"(
            {
              "size": 0,
              "query": {
              "function_score": {
                "random_score": {},
              "query": {
                "terms": {"hash": [)" << hashes << R"(]
                }}}},
              "aggs": {
              "sample": {
                "sampler": {
                  "shard_size": )" << shardSize << R"(
                },
                "aggs": {
                  "histogram": {
                    "terms": {
                    "field": "song_id",
                    "size": )" << focusBucketsCount << R"(
                  },
                    "aggs": {
                      "most_frequent": {
                        "top_hits": {
                          "size": )" << optimalWindowSize << R"(
                        }
                      }
                    }
                  }
                }
              }
             }
            }
            )";
            return std::regex_replace(ss.str(), std::regex(R"(\r\n|\r|\n)"), "");
        };

        const auto& hashes = snippet.get_hashes();
        QueryCollection queryCollection;
        queryCollection.reserve(hashes.size());

        std::stringstream stream;
        for (size_t i = 0; i < hashes.size(); i++)
        {
            stream << '\"' << hashes[i] << '\"';
            if (i % optimalBatchSize == 0 || i == hashes.size() - 1)
            {
                Query query;
                query.emplace("lucene", "fingerprint/_msearch");
                query.emplace("header", "{}");
                query.emplace("query", formQuery(stream.str()));
                query.emplace("request_type", "GET");

                queryCollection.insertQuery(std::move(query));
                stream.clear();
                stream.str({});
            }

            if (stream.rdbuf()->in_avail() != 0)
            {
                stream << ',';
            }
        }

        DBConnectionPtr elasticConnection = m_cachePool->getConnection();
        DBCommandPtr elasticCommand = elasticConnection->createCommand(std::move(queryCollection));
        bool success = elasticCommand->execute();
        m_cachePool->releaseConnection(std::move(elasticConnection));
        if (!success)
        {
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Could not fetch data from cache");
            isSuccess = false;
            return {};
        }
        isSuccess = true;
        return elasticCommand;
    }

    DBCommandPtr Engine::fetchFingerprintsFromPrimary(bool& isSuccess, const FingerprintType& snippet)
    {
        const auto& hashes = snippet.get_hashes();
        std::stringstream stream;
        stream << "SELECT hash, timestamp, song_id FROM fingerprint WHERE hash = ANY (\'{";

        for (size_t i = 0; i < hashes.size(); i++)
        {
            stream << hashes[i];
            if (i != hashes.size() - 1)
            {
                stream << ',';
            }
        }

        stream << "}'::numeric[]);";

        Query query;
        query.emplace("query", stream.str());

        DBConnectionPtr postgresConnection = m_primaryPool->getConnection();
        DBCommandPtr postgresCommand = postgresConnection->createCommand(std::move(query));
        bool success = postgresCommand->execute();
        m_primaryPool->releaseConnection(std::move(postgresConnection));
        if (!success)
        {
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Could not fetch data from primary storage");
            isSuccess = false;
            return {};
        }
        isSuccess = true;
        return postgresCommand;
    }

    HistReturnType Engine::findSongIdByFingerprint(bool& isSuccess, FingerprintType&& fingerprint)
    {
        bool isElasticSuccess = false;
        DBCommandPtr elasticCommand = fetchFingerprintsFromCache(isElasticSuccess, fingerprint);
        if (!isElasticSuccess)
        {
            isSuccess = false;
            return HistReturnType{HistStatus::Uncertain};
        }

        Histogram elasticHistogram(elasticCommand, fingerprint);
        HistReturnType elasticHist = elasticHistogram.findDominantPeak();
        if (elasticHist)
        {
            isSuccess = true;
            return elasticHist;
        }
        Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, "Failed to deduce song id from cache data");

        bool isPostgresSuccess = false;
        DBCommandPtr postgresCommand = fetchFingerprintsFromPrimary(isPostgresSuccess, fingerprint);
        if (!isPostgresSuccess)
        {
            isSuccess = false;
            return HistReturnType{HistStatus::Uncertain};
        }

        Histogram postgresHistogram(postgresCommand, fingerprint);
        HistReturnType postgresHist = postgresHistogram.findDominantPeak();
        if (postgresHist)
        {
            isSuccess = true;
            AsyncManager::instance().submitTask([&]
            {
                markAsyncStart();
                if (!cacheFingerprintBySongId(postgresHist.getSongId()))
                {
                    Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Failed to cache data");
                }
                markAsyncEnd();
            });
            return postgresHist;
        }
        Logger::log(LogLevel::WARNING, __FILE__, __FUNCTION__, __LINE__, "Failed to deduce song id from primary storage data");
        isSuccess = false;
        return HistReturnType{HistStatus::Uncertain};
    }

    bool Engine::isSongIdInPrimary(bool& exists, SongIdType songId, bool shouldClaim)
    {
        Query query;
        std::stringstream sql;
        if (shouldClaim)
        {
            sql << "SELECT \"find_song_id\"(" << songId << ")";
        }
        else
        {
            sql << "SELECT EXISTS (SELECT 1 from fingerprint WHERE song_id = " << songId << ')'
                << "AS \"find_song_id\";";
        }

        query.emplace("query", sql.str());

        DBConnectionPtr connection = m_primaryPool->getConnection();
        DBCommandPtr command = connection->createCommand(std::move(query));

        if (!command->execute() || !command->asBool("find_song_id", exists))
        {
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Failed to check whether song is in primary storage");
            m_primaryPool->releaseConnection(std::move(connection));
            return false;
        }
        m_primaryPool->releaseConnection(std::move(connection));
        return true;
    }

    bool Engine::isSongIdInCache(bool& exists, SongIdType songId)
    {
        Query query;
        std::stringstream stream;
        stream << R"({"query": {"match": {"song_id":)" << songId << "}}}";

        query.emplace("lucene", "fingerprint/_count");
        query.emplace("query", stream.str());
        query.emplace("request_type", "GET");

        DBConnectionPtr connection = m_cachePool->getConnection();
        DBCommandPtr command = connection->createCommand(std::move(query));

        size_t count{0};
        if (!command->execute() || !command->asSize("count", count))
        {
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Failed to check whether song is in cache");
            m_cachePool->releaseConnection(std::move(connection));
            return false;
        }
        m_cachePool->releaseConnection(std::move(connection));
        exists = count > 0;
        return true;
    }

    bool Engine::loadFingerprintIntoPrimary(const FingerprintType& fingerprint, SongIdType songId)
    {
        QueryCollection queryCollection;
        queryCollection.reserve(fingerprint.get_size());

        std::stringstream stream;
        for (auto it = fingerprint.cbegin(); it != fingerprint.cend(); it++)
        {
            Query query;

            stream << "INSERT INTO fingerprint(hash, song_id, timestamp) VALUES (";
            stream << it->first << ',' << songId << ',' << it->second << ')';

            query.emplace("query", stream.str());
            queryCollection.insertQuery(std::move(query));

            stream.clear();
            stream.str({});
        }

        bool isSuccess;
        DBConnectionPtr connection = m_primaryPool->getConnection();
        {
            DBCommandPtr command = connection->createCommand(std::move(queryCollection));
            isSuccess = command->execute();
        }

        // cleaning up in case there is a claiming row for this song_id
        Query cleanUpQuery;
        stream << "DELETE FROM fingerprint WHERE hash=-1 AND timestamp=-1 AND song_id=" << songId;
        cleanUpQuery.emplace("query", stream.str());

        DBCommandPtr command = connection->createCommand(std::move(cleanUpQuery));
        command->execute();

        m_primaryPool->releaseConnection(std::move(connection));
        return isSuccess;
    }

    bool Engine::loadFingerprintIntoCache(const FingerprintType& fingerprint, SongIdType songId)
    {
        QueryCollection queries;
        queries.reserve(fingerprint.get_size());

        std::stringstream stream;
        for (auto it = fingerprint.cbegin(); it != fingerprint.cend(); it++)
        {
            Query currentQuery;

            stream << "{\"hash\": " << it->first << ',';
            stream << "\"song_id\": " << songId << ',';
            stream << "\"timestamp\": " << it->second << '}';

            currentQuery.emplace("lucene", "fingerprint/_bulk");
            currentQuery.emplace("query", stream.str());
            currentQuery.emplace("request_type", "POST");
            currentQuery.emplace("header", "{\"index\": {}}");

            queries.insertQuery(std::move(currentQuery));
            stream.clear();
            stream.str({});
        }
        DBConnectionPtr connection = m_cachePool->getConnection();
        DBCommandPtr command = connection->createCommand(std::move(queries));
        bool isSuccess = command->execute();
        m_cachePool->releaseConnection(std::move(connection));
        return isSuccess;
    }

    bool Engine::loadTrackByUrl(const std::string& url, SongIdType songId, bool isCaching)
    {
        if (!isSongIdValid(songId))
        {
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Tried to load a song already in storage");
            return false;
        }

        std::string dir = "/tmp";
        std::string filePath;
        if (!generateUniqueFilePath(dir, filePath))
        {
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Failed to generate a unique file name");
            return false;
        }

        std::string useSslStr = siren::getenv("USE_SSL");
        bool useSsl = !useSslStr.empty() ? std::stoi(useSslStr) : 1;

        std::string strTimeout = siren::getenv("THIRDPARTY_API_TIMEOUT_MS");
        int timeout = !strTimeout.empty() ? std::stoi(strTimeout) : 30000;

        std::ofstream ofstream(filePath, std::ios::binary);
        const HttpResponse& res = RequestManager::DownloadFile(url, ofstream, timeout, useSsl);

        if (res.status_code == 0)
        {
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, res.error.message);
            return false;
        }
        if (res.status_code >= 400)
        {
            std::stringstream err;
            err << "Error ["<< res.status_code <<"] making request: " << res.text;
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, err.str());
            return false;
        }

        const CoreReturnType& coreResult = m_sirenCore->make_fingerprint(filePath);
        if (coreResult.code != siren::CoreStatus::OK)
        {
            std::stringstream err;
            err << "Could not fingerprint the track, status: " << (int)coreResult.code;
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, err.str());
            return false;
        }

        std::stringstream msg;
        msg << "Created a fingerprint of size " << coreResult.fingerprint.get_size() << " for track with id " << songId;
        Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, msg.str());

        {
            AsyncManager::instance().submitTask([this, fpt = coreResult.fingerprint, songId] {
                 markAsyncStart();
                 if (!loadFingerprintIntoPrimary(fpt, songId))
                 {
                     Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Failed to load fingerprint into primary");
                 }
                 markAsyncEnd();
            });

            if (!isCaching)
            {
                return true;
            }

            AsyncManager::instance().submitTask([this, fpt = coreResult.fingerprint, songId] {
                markAsyncStart();
                if (!loadFingerprintIntoCache(fpt, songId))
                {
                    Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Failed to load fingerprint into cache");
                }
                markAsyncEnd();
            });
        }
        return true;
    }

    bool Engine::purgeTrackFingerprintFromCache(SongIdType songId)
    {
        std::stringstream stream;
        Query query;

        stream << R"({"query": {"term": {"song_id": )" << songId << "}}}";

        query.emplace("lucene", "fingerprint/_delete_by_query");
        query.emplace("query", stream.str());
        query.emplace("request_type", "POST");

        DBConnectionPtr connection = m_cachePool->getConnection();
        DBCommandPtr command = connection->createCommand(std::move(query));

        bool isSuccess = command->execute();
        m_cachePool->releaseConnection(std::move(connection));
        return isSuccess;
    }

    bool Engine::purgeTrackFingerprintFromPrimary(SongIdType songId)
    {
        std::stringstream sql;
        Query query;

        sql << "DELETE FROM fingerprint WHERE song_id=" << songId;
        query.emplace("query", sql.str());

        DBConnectionPtr connection = m_primaryPool->getConnection();
        DBCommandPtr command = connection->createCommand(std::move(query));

        bool isSuccess = command->execute();
        m_cachePool->releaseConnection(std::move(connection));
        return isSuccess;
    }

    bool Engine::purgeFingerprintBySongId(SongIdType songId)
    {
        bool isPostgresSuccess = false;
        bool isElasticSuccess = false;
        {
            WaitableFuture postgresFuture = AsyncManager::instance().submitTask([&]{
                markAsyncStart();
                isPostgresSuccess = purgeTrackFingerprintFromPrimary(songId);
                markAsyncEnd();
                if (!isPostgresSuccess)
                {
                    Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Failed to delete fingerprint from primary");
                }
            }, true);

            WaitableFuture elasticFuture = AsyncManager::instance().submitTask([&]{
                markAsyncStart();
                isElasticSuccess = purgeTrackFingerprintFromCache(songId);
                markAsyncEnd();
                if (!isElasticSuccess)
                {
                    Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Failed to delete fingerprint from cache");
                }
            }, true);
        }
        return isPostgresSuccess && isElasticSuccess;
    }

    bool Engine::isSongIdValid(SongIdType songId)
    {
        bool primaryCheck;
        bool isPrimaryOk = isSongIdInPrimary(primaryCheck, songId, true);
        return isPrimaryOk && !primaryCheck;
    }

}// namespace siren::cloud
