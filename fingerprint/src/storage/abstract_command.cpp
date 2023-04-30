#include "abstract_command.h"

AbstractCommand::AbstractCommand(const Query& query)
    : m_queries{query}
{
}

AbstractCommand::AbstractCommand(Query&& query)
    : m_queries{std::move(query)}
{
}

AbstractCommand::AbstractCommand(QueryCollection&& queries)
    : m_queries{std::move(queries)}
{
    m_isBatch = true;
}

bool AbstractCommand::isBatch() const
{
    return m_isBatch;
}

QueryCollection AbstractCommand::getQueries() const
{
    return m_queries;
}

