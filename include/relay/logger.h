#ifndef RELAY_LOGGER_H
#define RELAY_LOGGER_H

#include <iostream>
#include <fstream>
#include <mutex>
#include <string>
#include <memory>
#include <map>

namespace relay
{
    /**
     * @enum LogLevel
     * @brief Log levels for controlling the verbosity of logging.
     */
    enum class LogLevel
    {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    /**
     * @class Logger
     * @brief Provides thread_safe logging functionality with different loglevels.
     *
     * The Logger class supports logging messages to the console, a file, or both,
     * with configurable log levels.
     */
    class Logger
    {
    public:
        /**
         * @brief Gets the singleton instance for the Logger
         * @return A reference to the logger instance.
         */
        static Logger &getInstance();

        /**
         * @brief Sets the minimum logging level. Messages below this level will not get logged.
         * @param level The minimum logging level.
         */
        void setLogLevel(LogLevel level);

        /**
         * @brief Enables logging to a file.
         * @param filename The file to write logs to.
         */
        void enableFileLogging(const std::string &filename);

        /**
         * @brief Logs a message with a specific log level.
         * @param level The log level.
         * @param message The message to log.
         */
        void log(LogLevel level, const std::string &message);

    private:
        Logger();
        ~Logger();
        Logger(const Logger &) = delete;
        Logger &operator=(const Logger &) = delete;

        std::mutex mutex_;
        LogLevel logLevel_;
        std::ofstream logFile_;
        bool fileLoggingEnabled_;

        const std::map<LogLevel, std::string> loglevelStrings_ = {
            {LogLevel::DEBUG, "DEBUG"},
            {LogLevel::INFO, "INFO"},
            {LogLevel::WARNING, "WARNING"},
            {LogLevel::ERROR, "ERROR"},
            {LogLevel::CRITICAL, "CRITICAL"},
        };
    };

}; // namespace relay

#endif