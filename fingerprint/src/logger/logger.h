#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

namespace siren::cloud
{
    enum class LogLevel
    {
        WARNING,
        INFO,
        ERROR,
        FATAL
    };

    class Logger
    {
    public:
        static void init(const std::string& filePath);
        static void log(LogLevel level, std::string_view file, std::string_view function, size_t line, std::string_view message);

    private:
        Logger() = default;
        inline static std::atomic<bool> m_initialised{false};
        inline static std::shared_ptr<spdlog::logger> m_logger;
    };
}