#pragma once
#include "abstract_command.h"

class AbstractConnection : public std::enable_shared_from_this<AbstractConnection>
{
public:
    explicit AbstractConnection(const std::string& connectionString);
    virtual ~AbstractConnection() = default;

    virtual bool open() = 0;
    virtual bool close() = 0;
    virtual bool isAlive() = 0;
    virtual bool tryRevive() = 0;

    virtual DBCommandPtr createCommand(const Query& query) = 0;
    virtual DBCommandPtr createCommand(Query&& query) = 0;
    virtual DBCommandPtr createCommand(QueryCollection&& queries) = 0;

protected:
    std::string getConnectionStr() const
    {
        return m_connectionString;
    }

private:
    const std::string m_connectionString;
};

using DBConnectionPtr = std::shared_ptr<AbstractConnection>;
