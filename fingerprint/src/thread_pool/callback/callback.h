#pragma once
#include <memory>

namespace siren::cloud
{
    class CallBack
    {
    public:
        template<typename Invocable>
        CallBack(Invocable invocable, size_t senderId)
            : m_self(std::make_unique<Model<Invocable>>(std::move(invocable)))
            , m_senderId(senderId)
        {
        }

        CallBack(const CallBack& other);
        CallBack(CallBack&& other) noexcept;
        CallBack& operator=(const CallBack& other);
        CallBack& operator=(CallBack&& other) noexcept = default;

        void call() const;
        void operator()() const;
        size_t getSenderId() const;

    private:
        struct Concept {
            virtual ~Concept() = default;
            virtual std::unique_ptr<Concept> copy() const = 0;
            virtual void call() const = 0;
        };

        template<typename Invocable>
        struct Model: Concept {
            Model(Invocable invocable)
                : m_invocable(std::move(invocable))
            {
            }

            std::unique_ptr<Concept> copy() const override
            {
                return std::make_unique<Model>(*this);
            }

            void call() const override
            {
                m_invocable();
            }

            Invocable m_invocable;
        };

        size_t m_senderId;
        std::unique_ptr<const Concept> m_self;
    };

    using CallBackPtr = std::shared_ptr<CallBack>;
}