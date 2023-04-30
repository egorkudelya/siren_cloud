#pragma once
#include "../abstract_connector.h"
#include "postgres_connection.h"


namespace siren::cloud::postgres
{

    class PostgresConnector : public AbstractConnector
    {
    public:
        explicit PostgresConnector(const std::string& connectionString);

        ConnectorType getConnectorType() const override;
        std::shared_ptr<AbstractConnection> createConnection() override;
    };

    using DBConnectorPtr = std::shared_ptr<PostgresConnector>;

}