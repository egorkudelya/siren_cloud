#include "postgres_connector.h"

namespace siren::cloud::postgres
{

    PostgresConnector::PostgresConnector(const std::string& connectionString)
        : AbstractConnector(connectionString)
    {
    }

    AbstractConnector::ConnectorType PostgresConnector::getConnectorType() const
    {
        return AbstractConnector::ConnectorType::Postgres;
    }

    std::shared_ptr<AbstractConnection> PostgresConnector::createConnection()
    {
        return std::make_shared<PostgresConnection>(this->getConnectionStr());
    }

}