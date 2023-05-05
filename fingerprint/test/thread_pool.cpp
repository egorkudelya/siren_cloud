#include "../src/thread_pool/async_manager.h"
#include <gtest/gtest.h>
#include <cmath>
#include <numeric>

auto job = [] {
    std::vector<size_t> numbers;
    for (size_t i = 0; i < 1000000; i++)
    {
        size_t t = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        size_t val = log2(i % 3) * (t % 8) / 2;
        numbers.push_back(val);
    }
    std::sort(numbers.begin(), numbers.end(), std::greater<>());
    std::cout << "--------job marker--------" << std::endl;
};


TEST(Pool, TestPause)
{
    auto pool = std::make_shared<siren::cloud::ThreadPool>(3, 3);
    auto thread = std::thread([&] {
        for (int i = 0; i < 1000; i++)
        {
            pool->submitTask(job);
        }
    });

    pool->pause();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    pool->resume();

    thread.join();
}

TEST(Pool, TestGracefulShutdown)
{
    auto pool = std::make_shared<siren::cloud::ThreadPool>(3, 2, true);
    auto thread = std::thread([&] {
        for (int i = 0; i < 3000; i++)
        {
            pool->submitTask(job);
        }
    });

    pool->pause();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    pool->resume();

    thread.join();
}

TEST(Pool, TestShutdown)
{
    auto pool = std::make_shared<siren::cloud::ThreadPool>(2, 4, false);
    auto thread = std::thread([&] {
        for (int i = 0; i < 3000; i++)
        {
            pool->submitTask(job);
        }
    });

    thread.join();
}

TEST(AsyncManager, TestNoWait)
{
    for (int i = 0; i < 1000; i++)
    {
        siren::cloud::AsyncManager::instance().submitTask(job);
    }
}

TEST(AsyncManager, TestWait)
{
    std::vector<siren::cloud::WaitableFuture> futures;
    for (int i = 0; i < 1000; i++)
    {
        futures.emplace_back(siren::cloud::AsyncManager::instance().submitTask(job, true));
    }
    std::cout << "waiting for completion" << std::endl;
}