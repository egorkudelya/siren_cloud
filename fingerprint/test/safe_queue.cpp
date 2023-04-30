#include <gtest/gtest.h>
#include "../src/thread_pool/primitives/waitable_future.h"
#include "../src/thread_pool/callback/callback.h"
#include "../src/thread_pool/async_manager.h"
#include "../src/common/safe_queue.h"

TEST(SafeQueue, TestTrivial)
{
    SafeQueue<siren::cloud::CallBack> queue{0};
    queue.push(siren::cloud::CallBack{[]{std::cout << "Simple test" << std::endl;}, 0});

    EXPECT_EQ(queue.getCurrentSize(), 1);

    siren::cloud::CallBack callback{[]{}, 0};
    ASSERT_TRUE(queue.pop(callback));
    ASSERT_TRUE(queue.isEmpty());
}

TEST(SafeQueue, TestParallel)
{
    SafeQueue<siren::cloud::CallBack> queue{0};
    size_t range = 1000000;
    size_t offset = 5;
    {
        auto producerFuture = siren::cloud::AsyncManager::instance().submitTask([&queue, range, offset] {
            siren::cloud::CallBack callback{[]{}, 0};
            for (size_t i = 0; i < range + offset; i++)
            {
                queue.push(callback);
            }
        }, true);

        auto consumerFuture = siren::cloud::AsyncManager::instance().submitTask([&queue, range] {
            siren::cloud::CallBack callback{[] {}, 0};
            for (size_t i = 0; i < range; i++)
            {
                queue.pop(callback);
            }
        }, true);
    }
    EXPECT_EQ(queue.getCurrentSize(), offset);
}