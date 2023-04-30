#pragma once

#include <memory>
#include <cassert>
#include "abstract_connector.h"
#include "../common/safe_queue.h"

namespace siren::cloud
{
    class DBConnectionPool
    {
    public:
        explicit DBConnectionPool(const DBConnectorPtr& connectorPtr, std::size_t poolSize = 10);
        ~DBConnectionPool();

        DBConnectionPtr getConnection();
        void releaseConnection(DBConnectionPtr connection);
        size_t getSize() const;

    private:
        bool init(std::size_t poolSize = 10);
        void close();

    private:
        DBConnectorPtr                 m_connector;
        SafeQueuePtr<DBConnectionPtr>  m_queue;
        std::atomic<size_t>            m_poolSize;
        std::atomic<bool>              m_isInitialized{false};
    };

    using DBConnectionPoolPtr = std::shared_ptr<DBConnectionPool>;
}