#include "logger.h"
#include <sstream>

namespace siren::cloud
{

    void Logger::init(const std::string& filePath)
    {
        m_logger = spdlog::daily_logger_mt("fingerprint_log", filePath);
        m_initialised = true;
    }

    void Logger::log(LogLevel level, std::string_view file, std::string_view function, size_t line, std::string_view message)
    {
        if (!m_initialised)
        {
            return;
        }
        std::stringstream completeMessage;
        completeMessage << file << " " << function << " " << line << " " << message;

        switch (level)
        {
            case LogLevel::INFO:
                m_logger->info(completeMessage.str());
                break;
            case LogLevel::WARNING:
                m_logger->warn(completeMessage.str());
                break;
            case LogLevel::ERROR:
                m_logger->error(completeMessage.str());
                break;
            case LogLevel::FATAL:
                m_logger->critical(completeMessage.str());
                throw std::runtime_error(completeMessage.str());
        }
        m_logger->flush();
    }

}