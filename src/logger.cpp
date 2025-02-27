#include "../include/relay/logger.h"

namespace relay
{

    Logger &Logger::getInstance()
    {
        static Logger instance;
        return instance;
    }

    Logger::Logger() : logLevel_(LogLevel::INFO), fileLoggingEnabled_(false) {}

    Logger::~Logger()
    {
        if (logFile_.is_open())
        {
            logFile_.close();
        }
    }

    void Logger::setLogLevel(LogLevel level)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        logLevel_ = level;
    }

    void Logger::enableFileLogging(const std::string &filename)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (logFile_.is_open())
        {
            logFile_.close();
        }
        logFile_.open(filename, std::ios::out | std::ios::app);
        if (!logFile_.is_open())
        {
            logFile_.clear();
            throw std::runtime_error("Failed to open file: " + filename);
        }
        fileLoggingEnabled_ = true;
    }

    void Logger::log(LogLevel level, const std::string &message)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (level < logLevel_)
        {
            return;
        }

        std::string logMessage = "[" + loglevelStrings_.at(level) + "] " + message;

        // Log to the console
        std::cout << logMessage << std::endl;

        // Log to the file if enabled
        if (fileLoggingEnabled_ && logFile_.is_open())
        {
            logFile_ << logMessage << '\n'; // Use '\n' for file logging
            logFile_.flush();               // Optional: Flush manually if needed for real-time logging
        }

        if (level == LogLevel::ERROR)
        {
            if (recentErrors_.size() >= MAX_ERRORS)
                recentErrors_.erase(recentErrors_.begin());
            recentErrors_.push_back("[" + timestamp() + "] " + message);
        }
    }

    std::vector<std::string> Logger::getRecentErrors() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return recentErrors_;
    }

    std::string Logger::timestamp() const
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return oss.string();
    }

} // namespace relay