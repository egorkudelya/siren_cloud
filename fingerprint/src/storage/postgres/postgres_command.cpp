#include "postgres_command.h"
#include "postgres_connection.h"

namespace siren::cloud::postgres
{
    PostgresCommand::PostgresCommand(const DBConnectionPtr& conn, const Query& query)
        : AbstractCommand(query)
        , m_connection(conn)
        , m_work(*(m_connection->getRawConnection()))
    {
    }

    PostgresCommand::PostgresCommand(const DBConnectionPtr& conn, Query&& query)
        : AbstractCommand(std::move(query))
        , m_connection(conn)
        , m_work(*(m_connection->getRawConnection()))
    {
    }

    PostgresCommand::PostgresCommand(const DBConnectionPtr& conn, QueryCollection&& queries)
        : AbstractCommand(std::move(queries))
        , m_connection(conn)
        , m_work(*(m_connection->getRawConnection()))
    {
    }

    bool PostgresCommand::isEmpty() const
    {
        return m_bufVec.empty();
    }

    bool PostgresCommand::execute()
    {
        pqxx::pipeline pipe(m_work);
        std::vector<size_t> ids;
        for (auto&& query: getQueries())
        {
            std::string sql = query.get("query");
            if (sql.empty())
            {
                Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "PGQuery passed to createCommand() lacks sql field");
                return false;
            }
            ids.push_back(pipe.insert(sql));
        }
         pipe.complete();
         m_work.commit();
         for (size_t id: ids)
         {
            pqxx::result result = pipe.retrieve(id);
            if (!result.empty())
            {
                m_bufVec.emplace_back(std::move(result));
            }
         }
        if (!m_bufVec.empty())
        {
            m_currBufIter = m_bufVec[m_idx].begin();
        }
        return true;
    }

    bool PostgresCommand::fetchNext()
    {
        if (m_bufVec.empty() || m_currBufIter == m_bufVec[m_bufVec.size()-1].end())
        {
            return false;
        }
        if (m_isFirstIter)
        {
            m_isFirstIter = false;
            return true;
        }
        if (m_currBufIter != m_bufVec[m_idx].end() - 1)
        {
            m_currBufIter++;
            return true;
        }
        if (m_idx < m_bufVec.size() - 1)
        {
            m_idx++;
            m_currBufIter = m_bufVec[m_idx].begin();
            return true;
        }
        return false;
    }

    size_t PostgresCommand::getSize() const
    {
        size_t outerSize = m_bufVec.size();
        if (outerSize > 0)
        {
            size_t innerSize = m_bufVec.begin()->size();
            return outerSize * innerSize;
        }
        return outerSize;
    }

    bool PostgresCommand::asInt32(const std::string& columnName, int32_t& val) const
    {
        return asValue<int32_t>(columnName, val);
    }

    bool PostgresCommand::asString(const std::string& columnName, std::string& val) const
    {
        return asValue<std::string>(columnName, val);
    }

    bool PostgresCommand::asFloat(const std::string& columnName, float& val) const
    {
        return asValue<float>(columnName, val);
    }

    bool PostgresCommand::asBool(const std::string& columnName, bool& val) const
    {
        return asValue<bool>(columnName, val);
    }

    bool PostgresCommand::asDouble(const std::string& columnName, double& val) const
    {
        return asValue<double>(columnName, val);
    }

    bool PostgresCommand::asUint64(const std::string& columnName, uint64_t& val) const
    {
        return asValue<uint64_t>(columnName, val);
    }

    bool PostgresCommand::asInt64(const std::string& columnName, int64_t& val) const
    {
        return asValue<int64_t>(columnName, val);
    }

    bool PostgresCommand::asSize(const std::string& columnName, size_t& val) const
    {
        return asValue<size_t>(columnName, val);
    }

    bool PostgresCommand::asShort(const std::string& columnName, short& val) const
    {
        return asValue<short>(columnName, val);
    }

}// namespace siren::service::postgres
