#include "src/service_umbrella.h"
#include "src/logger/logger.h"

void shouldQuit()
{
    std::mutex mtx;
    std::condition_variable cv;
    bool done{false};

    std::thread userThread([&]()
    {
        std::unique_lock<std::mutex> lock(mtx);
        std::cout << "Server is running. Press any key to gracefully shut it down." << std::endl;
        std::cin.get();
        std::cout << "Shutting down..." << std::endl;
        done = true;
        cv.notify_one();
    });

    std::unique_lock lock(mtx);
    cv.wait(lock, [&]{return done;});
    if (userThread.joinable())
    {
        userThread.join();
    }
}

int main()
{
    siren::cloud::Logger::init("../logs/log.txt");
    auto server = siren::cloud::CreateServer();
    server->Run();
    shouldQuit();
    server->ShutDown();
    return 0;
}