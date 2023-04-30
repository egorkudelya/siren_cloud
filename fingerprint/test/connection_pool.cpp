#include <gtest/gtest.h>
#include <sstream>
#include "common.h"
#include "../src/storage/connection_pool.h"
#include "../src/storage/postgres/postgres_connector.h"
#include "../src/thread_pool/async_manager.h"


TEST(DBConnectionPool, TestSimpleBorrowing)
{
    size_t initialSize = 7;
    std::string connStr = initPostgresConnStr();

    auto postgresConnector = std::make_shared<siren::cloud::postgres::PostgresConnector>(connStr);
    auto connectionPool = std::make_shared<siren::cloud::DBConnectionPool>(postgresConnector, initialSize);

    EXPECT_EQ(connectionPool->getSize(), initialSize);
    auto connection = connectionPool->getConnection();
    EXPECT_EQ(connectionPool->getSize(), initialSize - 1);
    connectionPool->releaseConnection(std::move(connection));
    EXPECT_EQ(connectionPool->getSize(), initialSize);
}

TEST(DBConnectionPool, TestParallelBorrowing)
{
    size_t initialSize = 100;
    std::string connStr = initPostgresConnStr();

    auto postgresConnector = std::make_shared<siren::cloud::postgres::PostgresConnector>(connStr);
    auto connectionPool = std::make_shared<siren::cloud::DBConnectionPool>(postgresConnector, initialSize);
    {
        Query query;
        query.emplace("query", "SELECT 1;");
        std::vector<WaitableFuture> futures;
        for (int i = 0; i < 100; i++)
        {
            futures.emplace_back(siren::cloud::AsyncManager::instance().submitTask([connectionPool, query] {
                auto connection = connectionPool->getConnection();
                auto command = connection->createCommand(query);
                connectionPool->releaseConnection(std::move(connection));
                release_assert(command->execute(), "command execution has failed");
            }, true));
        }
    }
    EXPECT_EQ(connectionPool->getSize(), initialSize);
}