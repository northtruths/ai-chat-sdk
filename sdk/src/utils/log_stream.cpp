#include "log_stream.hpp"
#include "logger.hpp"
#include "log_message.hpp"

namespace mylog
{
    LogStream::LogStream(Logger& owner, Level lv, const char* file, int line, Level danger_level)
        : owner_(owner), lv_(lv), file_(file), line_(line), danger_level_(danger_level) {}

    LogStream::~LogStream()
    {
        if (lv_ < owner_.min_level_.load(std::memory_order_acquire))
            return;
        LogMsg msg(lv_, file_, line_, oss_.str());
        owner_.transmitter_->send(owner_.formatter_->format(msg), owner_.sinks_, lv_ < danger_level_);
    }
}