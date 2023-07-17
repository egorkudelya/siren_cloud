#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

class Query
{

public:
    Query() = default;

    bool operator==(const Query& other) const;
    void emplace(const std::string& key, const std::string& value);
    void emplace(const std::string& key, std::string&& value);
    std::string get(const std::string& key) const;
    bool keyCompare(const Query& other) const;
    size_t getSize() const;

private:
    std::unordered_map<std::string, std::string> m_body;
};

class QueryCollection
{
public:
    using iterator = std::vector<Query>::iterator;
    using const_iterator = std::vector<Query>::const_iterator;

    QueryCollection() = default;
    QueryCollection(const Query& query);
    QueryCollection(iterator begin, iterator end);

    Query operator[](size_t i) const;
    iterator begin();
    iterator end();
    const_iterator cbegin() const;
    const_iterator cend() const;

    bool empty() const;
    size_t size() const;
    void reserve(size_t size);
    bool insertQuery(const Query& queryBody);
    bool insertQuery(Query&& queryBody);

private:
    bool checkIdentity(const Query& queryBody);

private:
    std::vector<Query> m_queries;
};