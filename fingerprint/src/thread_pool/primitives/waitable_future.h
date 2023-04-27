#pragma once
#include <future>

class WaitableFuture
{
public:
    template<typename T>
    explicit WaitableFuture(std::future<T>&& future, bool isWaiting)
    {
        m_future = std::make_unique<Model<std::future<T>>>(std::move(future));
        m_isWaiting = isWaiting;
    }

    WaitableFuture();
    ~WaitableFuture();
    WaitableFuture(WaitableFuture&& other) noexcept;
    WaitableFuture& operator=(WaitableFuture&& other) noexcept;
    WaitableFuture(const WaitableFuture& other) = delete;
    WaitableFuture& operator=(const WaitableFuture& other) = delete;

    bool valid() const;

private:
    struct Concept
    {
        virtual ~Concept() = default;
        virtual bool valid() const = 0;
        virtual void wait() = 0;
    };

    template <typename T>
    struct Model final : Concept
    {
        Model(T x) : m_future(std::move(x)) {}

        bool valid() const override
        {
            return m_future.valid();
        }

        void wait() override
        {
            m_future.wait();
        }

        T m_future;
    };

    bool m_isWaiting;
    std::unique_ptr<Concept> m_future;
};