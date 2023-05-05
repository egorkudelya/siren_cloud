#pragma once

#include "../abstract_connector.h"
#include "elastic_connection.h"

namespace siren::cloud::elastic
{
    class ElasticConnector : public AbstractConnector
    {
    public:
        explicit ElasticConnector(const std::string& connectionString);

        ConnectorType getConnectorType() const override;
        std::shared_ptr<AbstractConnection> createConnection() override;
    };

    using DBConnectorPtr = std::shared_ptr<ElasticConnector>;
}