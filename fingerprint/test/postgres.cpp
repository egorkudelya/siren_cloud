#include <iostream>
#include <gtest/gtest.h>
#include "common.h"
#include "../src/storage/postgres/postgres_connector.h"
#include "../src/storage/postgres/postgres_command.h"

bool initPostgresTestTable(const DBConnectionPtr& connectionPtr)
{
    Query tableQuery;
    tableQuery.emplace("query", "CREATE TABLE IF NOT EXISTS postgres_test(id INT, test INT);");
    auto command = connectionPtr->createCommand(std::move(tableQuery));
    return command->execute();
}

bool dropPostgresTestTable(const DBConnectionPtr& connectionPtr)
{
    Query dropTableQuery;
    dropTableQuery.emplace("query", "DROP TABLE postgres_test CASCADE;");
    auto command = connectionPtr->createCommand(std::move(dropTableQuery));
    return command->execute();
}

TEST(PostgresCommand, TestTrivial)
{
    std::string connStr = initPostgresConnStr();
    auto postgresConnector = std::make_shared<siren::cloud::postgres::PostgresConnector>(connStr);
    auto connection = postgresConnector->createConnection();

    ASSERT_TRUE(initPostgresTestTable(connection));

    Query insertQuery;
    std::stringstream insertStream;
    insertStream << "INSERT INTO postgres_test (id, test) VALUES (" << 0 << ','<< 3 << ')';
    insertQuery.emplace("query", insertStream.str());
    auto insertCommand = connection->createCommand(std::move(insertQuery));
    ASSERT_TRUE(insertCommand->execute());

    Query selectQuery;
    std::stringstream selectStream;
    selectStream << "SELECT test FROM postgres_test WHERE id = " << 0;
    selectQuery.emplace("query", selectStream.str());
    auto selectCommand = connection->createCommand(std::move(selectQuery));
    ASSERT_TRUE(selectCommand->execute());
    EXPECT_EQ(selectCommand->getSize(), 1);

    int val;
    ASSERT_TRUE(selectCommand->asInt32("test", val));
    EXPECT_EQ(val, 3);

    ASSERT_TRUE(dropPostgresTestTable(connection));
}

TEST(PostgresCommand, TestMany)
{
    std::string connStr = initPostgresConnStr();
    auto postgresConnector = std::make_shared<siren::cloud::postgres::PostgresConnector>(connStr);
    auto connection = postgresConnector->createConnection();

    ASSERT_TRUE(initPostgresTestTable(connection));

    size_t range = 100;
    for (size_t i = 0; i < range; i++)
    {
        Query insertQuery;
        std::stringstream insertStream;
        insertStream << "INSERT INTO postgres_test (id, test) VALUES (" << i << ',' << i << ')';
        insertQuery.emplace("query", insertStream.str());
        auto insertCommand = connection->createCommand(std::move(insertQuery));
        ASSERT_TRUE(insertCommand->execute());
    }

    Query selectQuery;
    QueryCollection queryCollection;
    std::stringstream selectStream;
    for (size_t i = 0; i < range; i++)
    {
        selectStream << "SELECT * FROM postgres_test WHERE id =" << i;
        selectQuery.emplace("query", selectStream.str());
        queryCollection.insertQuery(std::move(selectQuery));
        selectStream.clear();
        selectStream.str({});
    }
    auto selectCommand = connection->createCommand(std::move(queryCollection));
    ASSERT_TRUE(selectCommand->execute());
    EXPECT_EQ(selectCommand->getSize(), range);

    while (selectCommand->fetchNext())
    {
        int val;
        ASSERT_TRUE(selectCommand->asInt32("id", val));
        ASSERT_TRUE(selectCommand->asInt32("test", val));
    }

    ASSERT_TRUE(dropPostgresTestTable(connection));
}

TEST(PostgresCommand, TestSingleLargeResult)
{
    std::string connStr = initPostgresConnStr();
    auto postgresConnector = std::make_shared<siren::cloud::postgres::PostgresConnector>(connStr);
    auto connection = postgresConnector->createConnection();

    ASSERT_TRUE(initPostgresTestTable(connection));

    size_t range = 100;
    for (size_t i = 0; i < range; i++)
    {
        Query insertQuery;
        std::stringstream insertStream;
        insertStream << "INSERT INTO postgres_test (id, test) VALUES (" << i << ',' << i << ')';
        insertQuery.emplace("query", insertStream.str());
        auto insertCommand = connection->createCommand(std::move(insertQuery));
        ASSERT_TRUE(insertCommand->execute());
    }

    Query selectQuery;
    std::stringstream selectStream;
    selectStream << "SELECT * FROM postgres_test WHERE id IN (";
    for (size_t i = 0; i < range; i++)
    {
        selectStream << i;
        if (i != range - 1)
        {
            selectStream << ',';
        }
    }
    selectStream << ");";
    selectQuery.emplace("query", selectStream.str());
    auto selectCommand = connection->createCommand(std::move(selectQuery));
    ASSERT_TRUE(selectCommand->execute());
    EXPECT_EQ(selectCommand->getSize(), range);

    while (selectCommand->fetchNext())
    {
        int val;
        ASSERT_TRUE(selectCommand->asInt32("id", val));
        ASSERT_TRUE(selectCommand->asInt32("test", val));
    }

    ASSERT_TRUE(dropPostgresTestTable(connection));
}