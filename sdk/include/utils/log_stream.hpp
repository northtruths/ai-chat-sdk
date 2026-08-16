#pragma once
#include "log_level.hpp"
#include <sstream>
#include <string>

namespace mylog
{
    class Logger;

    class LogStream
    {
    public:
        LogStream(Logger &owner, Level lv, const char *file, int line, Level danger_level);
        ~LogStream();

        template <typename T>
        LogStream &operator<<(const T &value)
        {
            oss_ << value;
            return *this;
        }

    private:
        Logger &owner_;
        Level lv_;
        const char *file_;
        int line_;
        Level danger_level_;
        std::ostringstream oss_;
    };
}