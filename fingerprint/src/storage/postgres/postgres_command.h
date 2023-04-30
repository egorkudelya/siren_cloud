#pragma once

#include <pqxx/pqxx>
#include "../abstract_command.h"
#include "../../logger/logger.h"

namespace siren::cloud::postgres
{
    class PostgresConnection;
    using DBConnectionPtr = std::shared_ptr<PostgresConnection>;

    using Transaction = pqxx::work;
    using Buffer = pqxx::result;
    using BufferIter = Buffer::iterator;
    using BufferVec = std::vector<Buffer>;

    class PostgresCommand: public AbstractCommand
    {
    public:
        PostgresCommand(const DBConnectionPtr& conn, const Query& query);
        PostgresCommand(const DBConnectionPtr& conn, Query&& query);
        PostgresCommand(const DBConnectionPtr& conn, QueryCollection&& queries);

        [[nodiscard]] bool execute() override;
        bool isEmpty() const override;
        bool fetchNext() override;
        bool asInt32(const std::string& columnName, int32_t& val) const override;
        bool asString(const std::string& columnName, std::string& val) const override;
        bool asFloat(const std::string& columnName, float& val) const override;
        bool asBool(const std::string& columnName, bool& val) const override;
        bool asDouble(const std::string& columnName, double& val) const override;
        bool asUint64(const std::string& columnName, uint64_t& val) const override;
        bool asInt64(const std::string& columnName, int64_t& val) const override;
        bool asSize(const std::string& columnName, size_t& val) const override;
        bool asShort(const std::string& columnName, short& val) const override;
        size_t getSize() const override;

    private:
        template<typename T>
        bool asValue(const std::string& columnName, T& value) const
        {
            if (m_bufVec.empty())
            {
                Logger::log(LogLevel::WARNING, __FILE__, __FUNCTION__, __LINE__, "PGQuery did not return any results");
                return false;
            }
            try
            {
                pqxx::field field = m_currBufIter->at(columnName);
                if (!field.is_null())
                {
                    value = field.as<T>();
                    return true;
                }
                Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Requested PGField is null");
                return false;
            }
            catch (const std::exception& ex)
            {
                Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Requested PGField is not present in the result");
                return false;
            }
        }
    private:
        DBConnectionPtr m_connection;
        Transaction m_work;
        BufferVec m_bufVec;
        BufferIter m_currBufIter;
    };

}// namespace siren::service::postgres