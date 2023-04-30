#include "connection_pool.h"
#include "../logger/logger.h"
#include <sstream>

namespace siren::cloud
{
    DBConnectionPool::DBConnectionPool(const DBConnectorPtr& connectorPtr, std::size_t poolSize)
        : m_connector(connectorPtr)
        , m_poolSize(poolSize)
    {
        m_queue = std::make_shared<SafeQueue<DBConnectionPtr>>(0);
        init(poolSize);
    }

    DBConnectionPool::~DBConnectionPool()
    {
        close();
        m_isInitialized = false;
        Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, "DBConnectionPool has been shut down");
    }

    size_t DBConnectionPool::getSize() const
    {
        return m_poolSize;
    }

    DBConnectionPtr DBConnectionPool::getConnection()
    {
        auto createNewConnection = [this]
        {
            DBConnectionPtr conn = m_connector->createConnection();
            if (!conn->isAlive() && !conn->tryRevive())
            {
                Logger::log(LogLevel::FATAL, __FILE__, __FUNCTION__, __LINE__, "Connection created on the fly is invalid");
            }
            return conn;
        };

        if (!m_isInitialized)
        {
            Logger::log(LogLevel::WARNING, __FILE__, __FUNCTION__, __LINE__, "DBConnectionPool has not been explicitly initialized, initializing with size 10");
            init();
        }

        if (m_poolSize == 0)
        {
            Logger::log(LogLevel::WARNING, __FILE__, __FUNCTION__, __LINE__, "DBConnectionPool is empty, creating a new connection on the fly");
            return createNewConnection();
        }
        DBConnectionPtr conn;
        if (!m_queue->pop(conn))
        {
            Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Could not pop a DBConnection from the queue, creating a new one");
            return createNewConnection();
        }
        m_poolSize--;
        return conn;
    }

    void DBConnectionPool::releaseConnection(DBConnectionPtr connection)
    {
        if (connection)
        {
            if (!connection->isAlive() && !connection->tryRevive())
            {
                Logger::log(LogLevel::FATAL, __FILE__, __FUNCTION__, __LINE__, "Client returned a broken DBConnection");
            }
            m_queue->push(std::move(connection));
            m_poolSize++;
            return;
        }
        Logger::log(LogLevel::FATAL, __FILE__, __FUNCTION__, __LINE__, "Client returned an invalid DBConnection");
    }

    bool DBConnectionPool::init(std::size_t poolSize)
    {
        if (!m_queue->isEmpty())
        {
            Logger::log(LogLevel::WARNING, __FILE__, __FUNCTION__, __LINE__, "DBConnectionPool is not empty, reinitializing");
            close();
        }
        bool isInitialized = false;
        for (std::size_t i = 0; i < poolSize; i++)
        {
            DBConnectionPtr conn = m_connector->createConnection();
            if (conn->isAlive())
            {
                m_queue->push(std::move(conn));
                isInitialized = true;
            }
            if (!isInitialized)
            {
                break;
            }
            else if (m_queue->getCurrentSize() < poolSize - 1)
            {
                isInitialized = false;
            }
        }

        if (!isInitialized)
        {
            close();
        }
        m_isInitialized = isInitialized;
        m_poolSize = m_queue->getCurrentSize();

        if (m_isInitialized)
        {
            std::stringstream msg;
            msg << "DBConnectionPool has been initialized with size " << m_poolSize;
            Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, msg.str());
        }
        else
        {
            Logger::log(LogLevel::FATAL, __FILE__, __FUNCTION__, __LINE__, "DBConnectionPool initialization has failed");
        }
        return m_isInitialized;
    }

    void DBConnectionPool::close()
    {
        if (m_poolSize != m_queue->getCurrentSize())
        {
            Logger::log(LogLevel::FATAL, __FILE__, __FUNCTION__, __LINE__, "m_poolSize != m_pool.size()");
        }
        for (size_t i = 0; i < m_poolSize; i++)
        {
            DBConnectionPtr conn;
            bool isValid = m_queue->pop(conn);
            if (isValid)
            {
                conn->close();
            }
        }
        m_poolSize = m_queue->getCurrentSize();
        m_queue->signalAbort();
        if (m_poolSize != 0)
        {
            Logger::log(LogLevel::FATAL, __FILE__, __FUNCTION__, __LINE__, "DBConnectionPool is not empty after shutdown");
        }
    }

}// namespace siren::cloud