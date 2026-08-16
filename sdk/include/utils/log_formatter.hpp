// log_formatter.hpp
#pragma once
#include "log_message.hpp"
#include <string>

namespace mylog
{

    class Formatter
    {
    public:
        virtual ~Formatter() = default;
        virtual  std::string format(LogMsg &msg) = 0;
    };

    std::unique_ptr<Formatter> make_default_formatter();
} // namespace log