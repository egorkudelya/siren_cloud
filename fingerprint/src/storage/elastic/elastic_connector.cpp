#include "elastic_connector.h"

namespace siren::cloud::elastic
{

    ElasticConnector::ElasticConnector(const std::string& connectionString)
        : AbstractConnector(connectionString)
    {
    }

    AbstractConnector::ConnectorType ElasticConnector::getConnectorType() const
    {
        return AbstractConnector::ConnectorType::Elastic;
    }

    std::shared_ptr<AbstractConnection> ElasticConnector::createConnection()
    {
        return std::make_shared<ElasticConnection>(this->getConnectionStr());
    }

}