#pragma once
#include <memory>
#include "query.h"

class AbstractConnection;
using DBConnectionPtr = std::shared_ptr<AbstractConnection>;

class AbstractCommand
{
public:
    explicit AbstractCommand(const Query& query);
    explicit AbstractCommand(Query&& query);
    explicit AbstractCommand(QueryCollection&& queries);
    virtual ~AbstractCommand() = default;
    AbstractCommand(AbstractCommand&& other) noexcept = default;
    AbstractCommand& operator=(AbstractCommand&& other) noexcept = default;

    virtual bool isBatch() const;
    virtual QueryCollection getQueries() const;

    [[nodiscard]] virtual bool execute() = 0;
    virtual bool isEmpty() const = 0;
    virtual bool fetchNext() = 0;
    virtual bool asInt32(const std::string& columnName, int32_t& val) const = 0;
    virtual bool asString(const std::string& columnName, std::string& val) const = 0;
    virtual bool asFloat(const std::string& columnName, float& val) const = 0;
    virtual bool asBool(const std::string& columnName, bool& val) const = 0;
    virtual bool asDouble(const std::string& columnName, double& val) const = 0;
    virtual bool asUint64(const std::string& columnName, uint64_t& val) const = 0;
    virtual bool asInt64(const std::string& columnName, int64_t& val) const = 0;
    virtual bool asSize(const std::string& columnName, size_t& val) const = 0;
    virtual bool asShort(const std::string& columnName, short& val) const = 0;
    virtual size_t getSize() const = 0;

protected:
    size_t m_idx{0};
    bool m_isFirstIter{true};
    bool m_isBatch{false};
    QueryCollection m_queries;
};

using DBCommandPtr = std::shared_ptr<AbstractCommand>;
