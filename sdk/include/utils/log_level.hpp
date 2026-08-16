// log_level.hpp
#pragma once
#include <string>
namespace mylog
{
    enum class Level : int
    {
        TRACE = 0,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LEVELS
    };

    inline const char *level_to_string(Level lv)
    {
        switch (lv)
        {
        case Level::TRACE:
            return "TRACE";
        case Level::DEBUG:
            return "DEBUG";
        case Level::INFO:
            return "INFO";
        case Level::WARN:
            return "WARN";
        case Level::ERROR:
            return "ERROR";
        case Level::FATAL:
            return "FATAL";
        default:
            return "UNKNOWN";
        }
    }

    inline Level from_string(const std::string &s)
    {
        if (s == "TRACE")
            return Level::TRACE;
        if (s == "DEBUG")
            return Level::DEBUG;
        if (s == "INFO")
            return Level::INFO;
        if (s == "WARN")
            return Level::WARN;
        if (s == "ERROR")
            return Level::ERROR;
        if (s == "FATAL")
            return Level::FATAL;
        return Level::INFO; // 默认
    }
} // namespace log