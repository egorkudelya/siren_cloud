#include "elastic_command.h"
#include <memory>

namespace siren::cloud::elastic
{

    ElasticCommand::Credentials::Credentials()
    {
        elasticUser = siren::getenv("ELASTIC_USER");
        elasticPassword = siren::getenv("ELASTIC_PASSWORD");

        if (elasticUser.empty() || elasticPassword.empty())
        {
            Logger::log(LogLevel::FATAL, __FILE__, __FUNCTION__, __LINE__, "ES Credentials have not been set");
        }
    }

    ElasticCommand::ElasticCommand(const DBConnectionPtr& conn, const Query& query)
        : AbstractCommand(query)
        , m_connection(conn)
    {
    }

    ElasticCommand::ElasticCommand(const DBConnectionPtr& conn, Query&& query)
        : AbstractCommand(std::move(query))
        , m_connection(conn)
    {
    }

    ElasticCommand::ElasticCommand(const DBConnectionPtr& conn, QueryCollection&& queries)
        : AbstractCommand(std::move(queries))
        , m_connection(conn)
    {
    }

    static constexpr uint32_t hash(std::string_view data) noexcept
    {
        uint32_t hash = 5381;
        for (auto c: data)
        {
            hash = ((hash << 5) + hash) + (unsigned char)c;
        }
        return hash;
    }

    bool ElasticCommand::isEmpty() const
    {
        return m_bufVec.empty();
    }

    size_t ElasticCommand::getSize() const
    {
        return m_bufVec.size();
    }

    std::string ElasticCommand::formBulkQuery(Query&& queryBody, const std::string& header)
    {
        return {header + "\n" + queryBody.get("query") + "\n"};
    }

    bool ElasticCommand::execute()
    {
        Auth auth;
        auth.user = m_credentials.elasticUser;
        auth.password = m_credentials.elasticPassword;

        std::string useSslStr = siren::getenv("USE_SSL");
        bool useSsl = !useSslStr.empty() ? std::stoi(useSslStr) : 1;

        QueryCollection queries = getQueries();
        if (queries.empty())
        {
            Logger::log(LogLevel::WARNING, __FILE__, __FUNCTION__, __LINE__, "Tried to execute empty ESQuery");
            return true;
        }

        auto contains = [](std::string_view str, std::string_view suffix) {
            return str.find(suffix) != std::string::npos;
        };

        const std::string& connectionString = m_connection->getConnectionStr();
        const std::string& luceneQuery = queries.cbegin()->get("lucene");
        const std::string& type = queries.cbegin()->get("request_type");
        const std::string& header = queries.cbegin()->get("header");
        std::string url = connectionString + luceneQuery;
        size_t optimalBatchSize;

        if (luceneQuery.empty() || type.empty())
        {
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "ESQuery lacks lucene and/or request_type");
            return false;
        }

        if (queries.size() == 1)
        {
            return doExecute(auth, url, type, queries.begin()->get("query"), useSsl);
        }

        bool isBulk = contains(url, "_bulk") && !header.empty();
        bool isMultiSearch = contains(url, "_msearch") && !header.empty();

        if (isMultiSearch)
        {
            std::string batchSize = siren::getenv("ELASTIC_MULTI_BATCH_SIZE");
            optimalBatchSize = !batchSize.empty() ? std::stoi(batchSize) : 3;
        }
        else
        {
            std::string batchSize = siren::getenv("ELASTIC_BATCH_SIZE");
            optimalBatchSize = !batchSize.empty() ? std::stoul(batchSize) : 1000;
        }

        std::vector<WaitableFuture> futures;
        futures.reserve(queries.size());
        if (!isBulk && !isMultiSearch)
        {
            for (auto&& query: queries)
            {
                futures.emplace_back(AsyncManager::instance().submitTask(
                    [this, url, auth, type, useSsl, strQuery = std::move(query.get("query"))] {
                        if (!doExecute(auth, url, type, strQuery, useSsl))
                        {
                            Logger::log(LogLevel::FATAL, __FILE__, __FUNCTION__, __LINE__, "doExecute failed to execute ESQuery asynchronously."
                            "Consider making individual queries more lightweight or opting for a bulk version");
                        }
                    }, true));
            }
            return true;
        }

        if (header.empty())
        {
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "ESQuery lacks header to perform bulk/msearch request");
            return false;
        }

        if (queries.size() % optimalBatchSize != 0 || queries.size() < optimalBatchSize)
        {
            std::stringstream streamQuery;
            size_t i = queries.size() < optimalBatchSize ? 0 : floor(queries.size() / optimalBatchSize) * optimalBatchSize;
            for (; i < queries.size(); i++)
            {
                std::string currentQuery = formBulkQuery(std::move(queries[i]), header);
                if (currentQuery.empty())
                {
                    Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "formBulkQuery returned an invalid bulk ESQuery");
                    return false;
                }
                streamQuery << currentQuery;
            }

            if (!doExecute(auth, url, type, streamQuery.str(), useSsl))
            {
                Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "doExecute failed to execute ESQuery");
                return false;
            }

            if (queries.size() < optimalBatchSize)
            {
                return true;
            }
        }

        for (size_t i = 0; i <= queries.size() - optimalBatchSize; i += optimalBatchSize)
        {
            std::stringstream streamQuery;
            for (size_t j = i; j < i + optimalBatchSize; j++)
            {
                std::string currentQuery = formBulkQuery(std::move(queries[j]), header);
                if (currentQuery.empty())
                {
                    Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "formBulkQuery returned an invalid bulk ESQuery");
                    return false;
                }
                streamQuery << currentQuery;
            }
            futures.emplace_back(AsyncManager::instance().submitTask(
                [this, url, type, auth, useSsl, strQuery = std::move(streamQuery.str())] {
                    if (!doExecute(auth, url, type, strQuery, useSsl))
                    {
                        Logger::log(LogLevel::FATAL, __FILE__, __FUNCTION__, __LINE__,
                        "doExecute failed to execute ESQuery asynchronously. Consider adjusting the value for ELASTIC_BATCH_SIZE"
                        " and/or increasing memory limit for ES (MEM_LIMIT)");
                    }
                }, true));
        }
        return true;
    }

    bool ElasticCommand::doExecute(const Auth& auth, const std::string& url, const std::string& ReqType, const std::string& body, bool isVerifying)
    {
        std::string contentType = "application/json";
        HttpResponse res;
        switch (hash(ReqType))
        {
            case hash("PUT"):
                res = RequestManager::Put(url, body, contentType, auth, isVerifying);
                break;
            case hash("POST"):
                res = RequestManager::Post(url, body, contentType, auth, isVerifying);
                break;
            case hash("GET"):
                res = RequestManager::Get(url, body, contentType, auth, isVerifying);
                break;
            case hash("DELETE"):
                res = RequestManager::Delete(url, body, contentType, auth, isVerifying);
                break;
        }

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

        Json esResponse;
        try
        {
            esResponse = Json::parse(std::move(res.text));
        }
        catch (const Json::parse_error& error)
        {
            std::stringstream err;
            err << "Failed to parse ES response: " << error.what();
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, err.str());
            return false;
        }
        if (esResponse.contains("took"))
        {
            std::stringstream msg;
            msg << "ES query took " << esResponse["took"] << " ms";
            Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, msg.str());
        }
        if (esResponse.contains("error") || (esResponse.contains("errors") && esResponse["errors"].get<bool>()))
        {
            // TODO rollback
            std::stringstream err;
            err << "ES response has error(s): " << esResponse;
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, err.str());
            return false;
        }

        Json resArray;
        if (esResponse["hits"].contains("hits"))
        {
            resArray = esResponse["hits"]["hits"];
        }

        else if (esResponse.contains("responses"))
        {
            for (auto&& queryRes: esResponse["responses"])
            {
                Json hitsArray;
                if (queryRes.contains("aggregations") && queryRes["aggregations"].contains("sample") && queryRes["aggregations"]["sample"].contains("histogram"))
                {
                    for (auto&& bucket: queryRes["aggregations"]["sample"]["histogram"]["buckets"])
                    {
                        for (auto&& hit: bucket["most_frequent"]["hits"]["hits"])
                        {
                            hitsArray.emplace_back(std::move(hit));
                        }
                    }
                }
                else
                {
                    hitsArray = std::move(queryRes["hits"]["hits"]);
                }
                for (auto&& hit: hitsArray)
                {
                    resArray.emplace_back(std::move(hit));
                }
            }
        }

        if (esResponse.is_object() && resArray.empty())
        {
            resArray.emplace_back(std::move(esResponse));
        }

        std::lock_guard<std::mutex> lock(m_mtx);
        for (auto&& entry: resArray)
        {
            m_bufVec.emplace_back(std::move(entry));
        }
        return true;
    }

    bool ElasticCommand::fetchNext()
    {
        if (m_bufVec.empty())
        {
            return false;
        }
        if (m_isFirstIter)
        {
            m_isFirstIter = false;
            return true;
        }
        if (m_idx < m_bufVec.size() - 1)
        {
            m_idx++;
            return true;
        }
        return false;
    }

    bool ElasticCommand::asInt32(const std::string& fieldName, int32_t& val) const
    {
        return asValue<int32_t>(fieldName, val);
    }

    bool ElasticCommand::asString(const std::string& fieldName, std::string& val) const
    {
        return asValue<std::string>(fieldName, val);
    }

    bool ElasticCommand::asFloat(const std::string& fieldName, float& val) const
    {
        return asValue<float>(fieldName, val);
    }

    bool ElasticCommand::asBool(const std::string& fieldName, bool& val) const
    {
        return asValue<bool>(fieldName, val);
    }

    bool ElasticCommand::asDouble(const std::string& fieldName, double& val) const
    {
        return asValue<double>(fieldName, val);
    }

    bool ElasticCommand::asUint64(const std::string& fieldName, uint64_t& val) const
    {
        return asValue<uint64_t>(fieldName, val);
    }

    bool ElasticCommand::asInt64(const std::string& fieldName, int64_t& val) const
    {
        return asValue<int64_t>(fieldName, val);
    }

    bool ElasticCommand::asSize(const std::string& fieldName, size_t& val) const
    {
        return asValue<size_t>(fieldName, val);
    }

    bool ElasticCommand::asShort(const std::string& fieldName, short& val) const
    {
        return asValue<short>(fieldName, val);
    }

}// namespace siren::cloud::elastic