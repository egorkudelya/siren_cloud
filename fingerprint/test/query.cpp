#include <gtest/gtest.h>
#include "../src/storage/query.h"

TEST (Query, TestTrivial)
{
    QueryCollection queryCollection;
    Query query;
    query.emplace("sql", "SELECT 1;");
    ASSERT_TRUE(queryCollection.insertQuery(std::move(query)));
}

TEST(Query, TestCompatibility)
{
    QueryCollection queryCollection;

    Query first;
    first.emplace("first", "123");
    first.emplace("second", "text");
    first.emplace("third", "1.234");
    ASSERT_TRUE(queryCollection.insertQuery(std::move(first)));

    Query second;
    second.emplace("first", "...");
    second.emplace("third", "...");
    ASSERT_FALSE(queryCollection.insertQuery(std::move(second)));

    Query third;
    third.emplace("first", "...");
    third.emplace("second", "...");
    third.emplace("third", "...");
    ASSERT_TRUE(queryCollection.insertQuery(std::move(third)));
}