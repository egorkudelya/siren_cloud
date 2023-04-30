#pragma once
#include "../abstract_connection.h"

namespace siren::cloud::elastic
{

    class ElasticConnection: public AbstractConnection
    {
        friend class ElasticCommand;
    public:
        explicit ElasticConnection(const std::string& connectionString);

        bool open() override;
        bool close() override;
        bool isAlive() override;
        bool tryRevive() override;

        DBCommandPtr createCommand(const Query& query) override;
        DBCommandPtr createCommand(Query&& query) override;
        DBCommandPtr createCommand(QueryCollection&& queries) override;
    };

    using DBConnectionPtr = std::shared_ptr<ElasticConnection>;
}