#include "query.h"

bool Query::operator==(const Query& other) const
{
    return false;
}

void Query::emplace(const std::string& key, const std::string& value)
{
    m_body.emplace(key, value);
}

void Query::emplace(const std::string& key, std::string&& value)
{
    m_body.emplace(key, std::move(value));
}

size_t Query::getSize() const
{
    return m_body.size();
}

bool Query::keyCompare(const Query& other) const
{
    auto keyTraverser = [](const auto& map)
    {
        std::vector<std::string> keys;
        keys.reserve(map.size());
        for (auto [key, val]: map)
        {
            keys.push_back(key);
        }
        return keys;
    };

    return keyTraverser(m_body) == keyTraverser(other.m_body);
}

std::string Query::get(const std::string& key) const
{
    auto it = m_body.find(key);
    if (it != m_body.end())
    {
        return it ->second;
    }
    return {};
}


QueryCollection::QueryCollection(const Query& query)
    : m_queries{query}
{
}

QueryCollection::QueryCollection(QueryCollection::iterator begin, QueryCollection::iterator end)
{
    std::vector<Query> map{begin, end};
    m_queries = std::move(map);
}

Query QueryCollection::operator[](size_t i) const
{
    return m_queries[i];
}

QueryCollection::iterator QueryCollection::begin()
{
    return m_queries.begin();
}

QueryCollection::iterator QueryCollection::end()
{
    return m_queries.end();
}

QueryCollection::const_iterator QueryCollection::cbegin() const
{
    return m_queries.cbegin();
}

QueryCollection::const_iterator QueryCollection::cend() const
{
    return m_queries.cend();
}

bool QueryCollection::empty() const
{
    return m_queries.empty();
}

size_t QueryCollection::size() const
{
    return m_queries.size();
}

bool QueryCollection::checkIdentity(const Query& queryBody)
{
    if (m_queries.empty())
    {
        return true;
    }
    auto standard = m_queries.begin();
    if (!standard->keyCompare(queryBody))
    {
        return false;
    }
    return true;
}

bool QueryCollection::insertQuery(const Query& queryBody)
{
    if (!checkIdentity(queryBody))
    {
        return false;
    }
    m_queries.push_back(queryBody);
    return true;
}

bool QueryCollection::insertQuery(Query&& queryBody)
{
    if (!checkIdentity(queryBody))
    {
        return false;
    }
    m_queries.emplace_back(std::move(queryBody));
    return true;
}

void QueryCollection::reserve(size_t size)
{
    m_queries.reserve(size);
}
