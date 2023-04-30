#include "waitable_future.h"

namespace siren::cloud
{
    WaitableFuture::WaitableFuture()
    {
        std::future<void> defaultFuture;
        m_future = std::make_unique<Model<std::future<void>>>(std::move(defaultFuture));
        m_isWaiting = false;
    }

    WaitableFuture::WaitableFuture(WaitableFuture&& other) noexcept
    {
        m_future = std::move(other.m_future);
        m_isWaiting = other.m_isWaiting;
    }

    WaitableFuture& WaitableFuture::operator=(WaitableFuture&& other) noexcept
    {
        m_future = std::move(other.m_future);
        m_isWaiting = other.m_isWaiting;
        return *this;
    }

    bool WaitableFuture::valid() const
    {
        return m_future->valid();
    }

    WaitableFuture::~WaitableFuture()
    {
        if (m_isWaiting && m_future && m_future->valid())
        {
            m_future->wait();
        }
    }
}