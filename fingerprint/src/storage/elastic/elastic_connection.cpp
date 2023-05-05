#include "elastic_connection.h"
#include "elastic_command.h"

namespace siren::cloud::elastic
{
    ElasticConnection::ElasticConnection(const std::string& connectionString)
        : AbstractConnection(connectionString)
    {
    }

    bool ElasticConnection::open()
    {
        return true;
    }

    bool ElasticConnection::close()
    {
        return true;
    }

    bool ElasticConnection::isAlive()
    {
        return true;
    }

    bool ElasticConnection::tryRevive()
    {
        return true;
    }

    DBCommandPtr ElasticConnection::createCommand(const Query& query)
    {
        auto connectionPtr = std::static_pointer_cast<ElasticConnection>(shared_from_this());
        return std::make_shared<ElasticCommand>(connectionPtr, query);
    }

    DBCommandPtr ElasticConnection::createCommand(Query&& query)
    {
        auto connectionPtr = std::static_pointer_cast<ElasticConnection>(shared_from_this());
        return std::make_shared<ElasticCommand>(connectionPtr, std::move(query));
    }

    DBCommandPtr ElasticConnection::createCommand(QueryCollection&& queries)
    {
        auto connectionPtr = std::static_pointer_cast<ElasticConnection>(shared_from_this());
        return std::make_shared<ElasticCommand>(connectionPtr, std::move(queries));
    }

}