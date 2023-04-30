#include "abstract_connector.h"

AbstractConnector::AbstractConnector(const std::string& connectionString)
    : m_connectionString(connectionString)
{
}

std::string AbstractConnector::getConnectionStr() const
{
    return m_connectionString;
}
