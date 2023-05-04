#include "service_umbrella.h"
#include "storage/elastic/elastic_connector.h"
#include "storage/postgres/postgres_connector.h"

namespace siren::cloud
{

    std::unique_ptr<ServerImpl> CreateServer()
    {
        std::string servAddress = siren::getenv("SERV_ADDRESS");
        release_assert(!servAddress.empty(), "Server address is not provided. Make sure to set SERV_ADDRESS");

        std::string postgresPoolSizeStr = siren::getenv("POSTGRES_POOL_SIZE");
        std::string elasticPoolSizeStr = siren::getenv("ELASTIC_POOL_SIZE");

        size_t postgresPoolSize = !postgresPoolSizeStr.empty() ? std::stoul(postgresPoolSizeStr) : 50;
        size_t elasticPoolSize =  !elasticPoolSizeStr.empty() ? std::stoul(elasticPoolSizeStr) : 100;

        EngineParameters params;
        auto postgresConnector = std::make_shared<postgres::PostgresConnector>(params.postgresConnString);
        auto postgresPool = std::make_shared<DBConnectionPool>(postgresConnector, postgresPoolSize);

        auto elasticConnector = std::make_shared<elastic::ElasticConnector>(params.elasticConnString);
        auto elasticPool = std::make_shared<DBConnectionPool>(elasticConnector, elasticPoolSize);

        auto rawCore = siren_core::CreateCore();
        SirenCorePtr corePtr = std::shared_ptr<siren::SirenCore>(rawCore);

        auto engine = std::make_shared<Engine>(postgresPool, elasticPool, corePtr);
        return std::make_unique<ServerImpl>(servAddress, engine);
    }

}