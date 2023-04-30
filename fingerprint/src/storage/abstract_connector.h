#pragma once
#include "abstract_connection.h"

class AbstractConnector
{

public:
    explicit AbstractConnector(const std::string& connectionString);
    virtual ~AbstractConnector() = default;

    enum class ConnectorType
    {
        Postgres,
        DynamoDB,
        Elastic
    };

    virtual ConnectorType getConnectorType() const = 0;
    virtual DBConnectionPtr createConnection() = 0;

protected:
    std::string getConnectionStr() const;

private:
    const std::string m_connectionString;
};

using DBConnectorPtr = std::shared_ptr<AbstractConnector>;
