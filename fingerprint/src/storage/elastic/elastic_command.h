#pragma once

#include <mutex>
#include "elastic_connection.h"
#include "../../common/request_manager.h"
#include "../../thread_pool/async_manager.h"
#include "../../logger/logger.h"
#include "../../common/common.h"

namespace siren::cloud::elastic
{

    class ElasticCommand: public AbstractCommand
    {
    public:
        ElasticCommand(const DBConnectionPtr& conn, const Query& query);
        ElasticCommand(const DBConnectionPtr& conn, Query&& query);
        ElasticCommand(const DBConnectionPtr& conn, QueryCollection&& queries);

        [[nodiscard]] bool execute() override;
        bool isEmpty() const override;
        bool fetchNext() override;
        bool asInt32(const std::string& fieldName, int32_t& val) const override;
        bool asString(const std::string& fieldName, std::string& val) const override;
        bool asFloat(const std::string& fieldName, float& val) const override;
        bool asBool(const std::string& fieldName, bool& val) const override;
        bool asDouble(const std::string& fieldName, double& val) const override;
        bool asUint64(const std::string& fieldName, uint64_t& val) const override;
        bool asInt64(const std::string& fieldName, int64_t& val) const override;
        bool asSize(const std::string& fieldName, size_t& val) const override;
        bool asShort(const std::string& fieldName, short& val) const override;
        size_t getSize() const override;

    private:
        std::string formBulkQuery(Query&& queryBody, const std::string& header);
        bool doExecute(const Auth& auth, const std::string& url, const std::string& ReqType, const std::string& body);

        template<typename T>
        bool asValue(const std::string& fieldName, T& value) const
        {
            if (m_bufVec.empty())
            {
                Logger::log(LogLevel::WARNING, __FILE__, __FUNCTION__, __LINE__, "ESQuery did not return any results");
                return false;
            }
            Json currentJson;
            if (m_bufVec.at(m_idx).contains("_source"))
            {
                currentJson = m_bufVec.at(m_idx)["_source"];
            }
            else
            {
                currentJson = m_bufVec.at(m_idx);
            }
            if (currentJson.contains(fieldName))
            {
                value = currentJson.at(fieldName).get<T>();
                return true;
            }
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Could not extract ESQuery result");
            return false;
        }

        struct Credentials
        {
            Credentials();
            std::string elasticUser;
            std::string elasticPassword;
        };
    private:
        std::mutex m_mtx;
        Credentials m_credentials;
        Json m_bufVec;
        DBConnectionPtr m_connection;
    };

}// namespace siren::service::elastic