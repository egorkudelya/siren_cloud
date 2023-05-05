#include "engine.h"
#include "../thread_pool/async_manager.h"
#include "../common/request_manager.h"
#include "../logger/logger.h"

namespace siren_core
{
    siren::SirenCore* CreateCore()
    {
        siren::CoreSpecification spec;
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

        auto postgresConnection = m_primaryPool->getConnection();
        Query postgresReq;
        std::string sql = "SELECT hash, timestamp, song_id FROM fingerprint WHERE song_id = " + std::to_string(songId);
        postgresReq.emplace("query", sql);

        auto postgresCommand = postgresConnection->createCommand(std::move(postgresReq));
        if (!postgresCommand->execute() || postgresCommand->isEmpty())
        {
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Could not fetch data from primary storage for caching");
            m_primaryPool->releaseConnection(std::move(postgresConnection));
            return false;
        }

        QueryCollection elasticQueries;
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

        auto elasticConnection = m_cachePool->getConnection();
        auto elasticCommand = elasticConnection->createCommand(std::move(elasticQueries));

        bool isSuccess = elasticCommand->execute();

        m_cachePool->releaseConnection(std::move(elasticConnection));
        m_primaryPool->releaseConnection(std::move(postgresConnection));

        return isSuccess;
    }

    DBCommandPtr Engine::fetchFingerprintsFromCache(bool& isSuccess, const FingerprintType& snippet)
    {
        std::string batchSize = siren::getenv("ELASTIC_BATCH_SIZE");
        size_t optimalBatchSize = !batchSize.empty() ? std::stoul(batchSize) : 5000;

        auto formQuery = [](const std::string& hashes)
        {
            std::stringstream ss;
            ss << R"({"size": 10000,"query": {"constant_score" : {"filter" : {"terms" : {"hash" : [)" << hashes << "]}}}}}";
            return ss.str();
        };

        auto hashes = snippet.get_hashes();
        std::stringstream stream;
        QueryCollection queryCollection;
        for (size_t i = 0; i < hashes.size(); i++)
        {
            stream << '\"' << hashes[i] << '\"';
            if ((i % optimalBatchSize == 0 || i == hashes.size() - 1))
            {
                std::string str = formQuery(stream.str());
                Query query;
                query.emplace("lucene", "fingerprint/_msearch");
                query.emplace("header", "{}");
                query.emplace("query", str);
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

        auto elasticConnection = m_cachePool->getConnection();
        auto elasticCommand = elasticConnection->createCommand(std::move(queryCollection));
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
        auto hashes = snippet.get_hashes();
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

        auto postgresConnection = m_primaryPool->getConnection();
        auto postgresCommand = postgresConnection->createCommand(std::move(query));
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
        auto elasticCommand = fetchFingerprintsFromCache(isElasticSuccess, fingerprint);
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
        auto postgresCommand = fetchFingerprintsFromPrimary(isPostgresSuccess, fingerprint);
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

    bool Engine::isSongIdInPrimary(bool& exists, SongIdType songId)
    {
        Query query;
        std::stringstream sql;
        sql << "SELECT EXISTS (SELECT 1 from fingerprint WHERE song_id = " << songId << ')' << "AS \"exists\"";
        query.emplace("query", sql.str());

        auto connection = m_primaryPool->getConnection();
        auto command = connection->createCommand(std::move(query));

        if (!command->execute() || !command->asBool("exists", exists))
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

        auto connection = m_cachePool->getConnection();
        auto command = connection->createCommand(std::move(query));

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
        bool exists;
        bool isOk = isSongIdInPrimary(exists, songId);

        if (!isOk)
        {
            return false;
        }
        if (exists)
        {
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Tried to load a song already in primary storage");
            return false;
        }

        std::stringstream stream;
        QueryCollection queryCollection;
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
        auto connection = m_primaryPool->getConnection();
        auto command = connection->createCommand(std::move(queryCollection));

        bool isSuccess = command->execute();
        m_primaryPool->releaseConnection(std::move(connection));
        return isSuccess;
    }

    bool Engine::loadFingerprintIntoCache(const FingerprintType& fingerprint, SongIdType songId)
    {
        QueryCollection queries;
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
        auto connection = m_cachePool->getConnection();
        auto command = connection->createCommand(std::move(queries));
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

        std::ofstream ofstream(filePath);
        HttpResponse res = RequestManager::DownloadFile(url, ofstream);

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

        auto coreResult = m_sirenCore->make_fingerprint(filePath);
        if (coreResult.code != siren::CoreStatus::OK)
        {
            std::stringstream err;
            err << "Could not fingerprint a track, status: " << (int)coreResult.code;
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, err.str());
            return false;
        }
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

        auto connection = m_cachePool->getConnection();
        auto command = connection->createCommand(std::move(query));

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

        auto connection = m_primaryPool->getConnection();
        auto command = connection->createCommand(std::move(query));

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
        bool cacheCheck;
        bool isCacheOk = isSongIdInCache(cacheCheck, songId);

        if (!isCacheOk || cacheCheck)
        {
           return false;
        }

        bool primaryCheck;
        bool isPrimaryOk = isSongIdInPrimary(primaryCheck, songId);

        if (!isPrimaryOk || primaryCheck)
        {
            return false;
        }
        return true;
    }

}// namespace siren::cloud
