#include <iostream>
#include <sstream>
#include <gtest/gtest.h>
#include "../src/storage/elastic/elastic_connector.h"
#include "../src/storage/elastic/elastic_command.h"
#include "common.h"

bool deleteIndex(const DBConnectionPtr& connectionPtr)
{
    Query query;
    query.emplace("lucene", "test/_delete_by_query");
    query.emplace("query", R"({"query": {"match_all": {}}})");
    query.emplace("request_type", "POST");

    auto command = connectionPtr->createCommand(std::move(query));
    return command->execute();
}

TEST(ElasticSearch, TestTrivial)
{
    std::string connStr = initElasticConnStr();
    auto elasticConnector = std::make_shared<siren::cloud::elastic::ElasticConnector>(connStr);
    auto connection = elasticConnector->createConnection();

    Query insertQuery;
    insertQuery.emplace("lucene", "test/_doc");
    insertQuery.emplace("query", R"({"int": 55, "string": "Hello world", "bool": true})");
    insertQuery.emplace("request_type", "POST");

    auto insertCommand = connection->createCommand(std::move(insertQuery));
    ASSERT_TRUE(insertCommand->execute());

    std::this_thread::sleep_for(std::chrono::seconds(1));

    Query searchQuery;
    searchQuery.emplace("lucene", "test/_search?q=int:55&pretty");
    searchQuery.emplace("request_type", "GET");

    auto searchCommand = connection->createCommand(std::move(searchQuery));
    ASSERT_TRUE(searchCommand->execute());
    EXPECT_EQ(searchCommand->getSize(), 1);

    int i;
    std::string s;
    bool b;
    ASSERT_TRUE(searchCommand->asInt32("int", i));
    ASSERT_TRUE(searchCommand->asString("string", s));
    ASSERT_TRUE(searchCommand->asBool("bool", b));

    EXPECT_EQ(i, 55);
    EXPECT_EQ(s, "Hello world");
    EXPECT_EQ(b, true);

    deleteIndex(connection);
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST(ElasticSearch, TestMany)
{
    std::string connStr = initElasticConnStr();
    auto elasticConnector = std::make_shared<siren::cloud::elastic::ElasticConnector>(connStr);
    auto connection = elasticConnector->createConnection();

    std::stringstream stream;
    size_t range = 12;
    for (size_t i = 0; i < range; i++)
    {
        Query insertQuery;

        stream << R"({"int": )" << i << ',' << R"("string": "Hello world", "bool": true})";
        insertQuery.emplace("lucene", "test/_doc");
        insertQuery.emplace("query", stream.str());
        insertQuery.emplace("request_type", "POST");

        auto insertCommand = connection->createCommand(std::move(insertQuery));
        ASSERT_TRUE(insertCommand->execute());

        stream.clear();
        stream.str({});
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    Query searchQuery;
    searchQuery.emplace("lucene", "test/_search");
    searchQuery.emplace("query", R"({"size": 10000,"query": {"match": {"string": {"query": "Hello world"}}}})");
    searchQuery.emplace("request_type", "GET");

    auto searchCommand = connection->createCommand(std::move(searchQuery));
    ASSERT_TRUE(searchCommand->execute());
    EXPECT_EQ(searchCommand->getSize(), range);

    deleteIndex(connection);
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST(ElasticSearch, TestBulk)
{
    std::string connStr = initElasticConnStr();
    auto elasticConnector = std::make_shared<siren::cloud::elastic::ElasticConnector>(connStr);
    auto connection = elasticConnector->createConnection();

    size_t range = 500;
    std::stringstream stream;
    QueryCollection queryCollection;
    for (size_t i = 0; i < range; i++)
    {
        Query insertQuery;

        stream << R"({"int":)" << i << ',' << R"("string": "Hello world", "bool": true})";
        insertQuery.emplace("lucene", "test/_bulk");
        insertQuery.emplace("header", "{\"index\": {}}");
        insertQuery.emplace("query", stream.str());
        insertQuery.emplace("request_type", "POST");

        queryCollection.insertQuery(std::move(insertQuery));

        stream.clear();
        stream.str({});
    }

    auto insertCommand = connection->createCommand(std::move(queryCollection));
    ASSERT_TRUE(insertCommand->execute());

    std::this_thread::sleep_for(std::chrono::seconds(1));

    Query searchQuery;
    searchQuery.emplace("lucene", "test/_search");
    searchQuery.emplace("query", R"({"size": 10000,"query": {"match": {"string": {"query": "Hello world"}}}})");
    searchQuery.emplace("request_type", "GET");

    auto searchCommand = connection->createCommand(std::move(searchQuery));
    ASSERT_TRUE(searchCommand->execute());
    EXPECT_EQ(searchCommand->getSize(), range);

    auto formMSearchQuery = [](const std::string& ints)
    {
        std::stringstream ss;
        ss << R"({"size": 10000,"query": {"constant_score": {"filter": {"terms": {"int": [)" << ints << "]}}}}}";
        return ss.str();
    };

    std::stringstream ints;
    QueryCollection msearchCollection;
    for (size_t i = 0; i < range; i++)
    {
        ints << i;
        if (i % 50 == 0 || i == range - 1)
        {
            Query query;
            query.emplace("lucene", "test/_msearch");
            query.emplace("header", "{}");
            query.emplace("query", formMSearchQuery(ints.str()));
            query.emplace("request_type", "GET");

            msearchCollection.insertQuery(std::move(query));
            ints.clear();
            ints.str({});
        }

        if (ints.rdbuf()->in_avail() != 0)
        {
            ints << ',';
        }
    }

    auto mSearchCommand = connection->createCommand(std::move(msearchCollection));
    ASSERT_TRUE(mSearchCommand->execute());
    EXPECT_EQ(mSearchCommand->getSize(), range);

    deleteIndex(connection);
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST(ElasticSearch, CountApi)
{
    std::string connStr = initElasticConnStr();
    auto elasticConnector = std::make_shared<siren::cloud::elastic::ElasticConnector>(connStr);
    auto connection = elasticConnector->createConnection();

    size_t range = 500;
    std::stringstream stream;
    QueryCollection queryCollection;
    for (size_t i = 0; i < range; i++)
    {
        Query insertQuery;

        stream << R"({"int": )" << i << ',' << R"("string": "Hello world", "bool": true})";
        insertQuery.emplace("lucene", "test/_bulk");
        insertQuery.emplace("header", "{\"index\": {}}");
        insertQuery.emplace("query", stream.str());
        insertQuery.emplace("request_type", "POST");

        queryCollection.insertQuery(std::move(insertQuery));

        stream.clear();
        stream.str({});
    }

    auto insertCommand = connection->createCommand(std::move(queryCollection));
    ASSERT_TRUE(insertCommand->execute());

    std::this_thread::sleep_for(std::chrono::seconds(1));

    Query query;
    std::stringstream countStream;
    countStream << R"({"query": {"match": {"string":)" << R"("Hello world")" << "}}}";

    query.emplace("lucene", "test/_count");
    query.emplace("query", countStream.str());
    query.emplace("request_type", "GET");

    auto countCommand = connection->createCommand(std::move(query));
    ASSERT_TRUE(countCommand->execute());

    size_t count{0};
    ASSERT_TRUE(countCommand->asSize("count", count));
    EXPECT_EQ(count, range);

    deleteIndex(connection);
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
