#pragma once
#include <pqxx/pqxx>
#include "../abstract_connection.h"

namespace siren::cloud::postgres
{

    using Connection = pqxx::connection;

    class PostgresConnection: public AbstractConnection
    {
        friend class PostgresCommand;
    public:
        explicit PostgresConnection(const std::string& connectionString);

        bool open() override;
        bool close() override;
        bool isAlive() override;
        bool tryRevive() override;

        DBCommandPtr createCommand(const Query& query) override;
        DBCommandPtr createCommand(Query&& query) override;
        DBCommandPtr createCommand(QueryCollection&& queries) override;

    private:
        Connection* getRawConnection();

    private:
        Connection m_connection;
    };

}