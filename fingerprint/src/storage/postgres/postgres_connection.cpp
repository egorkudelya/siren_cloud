#include "postgres_connection.h"
#include "postgres_command.h"

namespace siren::cloud::postgres
{

    PostgresConnection::PostgresConnection(const std::string& connectionString)
        : AbstractConnection(connectionString)
        , m_connection(connectionString)
    {
    }

    bool PostgresConnection::open()
    {
        if (isAlive())
        {
            return true;
        }
        m_connection = pqxx::connection{getConnectionStr()};
        return m_connection.is_open();
    }

    bool PostgresConnection::close()
    {
        m_connection.close();
        return !m_connection.is_open();
    }

    bool PostgresConnection::isAlive()
    {
        return m_connection.is_open();
    }

    bool PostgresConnection::tryRevive()
    {
        if (!isAlive())
        {
            open();
            return isAlive();
        }
        return true;
    }

    DBCommandPtr PostgresConnection::createCommand(const Query& query)
    {
        auto connectionPtr = std::static_pointer_cast<PostgresConnection>(shared_from_this());
        return std::make_shared<PostgresCommand>(connectionPtr, query);
    }

    DBCommandPtr PostgresConnection::createCommand(Query&& query)
    {
        auto connectionPtr = std::static_pointer_cast<PostgresConnection>(shared_from_this());
        return std::make_shared<PostgresCommand>(connectionPtr, std::move(query));
    }

    DBCommandPtr PostgresConnection::createCommand(QueryCollection&& queries)
    {
        auto connectionPtr = std::static_pointer_cast<PostgresConnection>(shared_from_this());
        return std::make_shared<PostgresCommand>(connectionPtr, std::move(queries));
    }

    Connection* PostgresConnection::getRawConnection()
    {
        return &m_connection;
    }

}