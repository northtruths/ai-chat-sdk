// log_transmitter.hpp
#pragma once
#include "log_sink.hpp"
#include <vector>
#include <memory>
#include <string>

namespace mylog
{
    class Transmitter
    {
    public:
        virtual ~Transmitter() = default;

        virtual void send(const std::string& formatted_msg,
                          std::vector<std::unique_ptr<Sink>> &sinks, bool is_safe) = 0;
    };

    std::unique_ptr<Transmitter> make_sync_transmitter();
    std::unique_ptr<Transmitter> make_async_transmitter(size_t buffer_size = 1 * 1024 * 1024, int flush_interval_ms = 1000);

} // namespace log