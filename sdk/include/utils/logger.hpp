// logger.hpp
#pragma once
#include "log_level.hpp"
#include "log_message.hpp"
#include "log_formatter.hpp"
#include "log_sink.hpp"
#include "log_transmitter.hpp"
#include "log_stream.hpp"
#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <sstream>
#include <signal.h>

namespace mylog
{
    class Logger
    {
        friend class LogStream;

    public:
        // 注册信号处理（在 main 里调用一次）
        void register_signal_handler()
        {
            struct sigaction sa;
            sa.sa_handler = signal_handler;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            sigaction(SIGINT, &sa, nullptr);  // Ctrl+C
            sigaction(SIGTERM, &sa, nullptr); // kill
            sigaction(SIGSEGV, &sa, nullptr); // 段错误（可选）
        }

        // 检查是否收到退出信号
        bool should_exit() const
        {
            return exit_requested_.load();
        }

        // 强制刷新所有缓冲区（信号处理函数里调用）
        void flush_all()
        {
            for (auto &sink : sinks_)
            {
                if (sink)
                {
                    sink->flush();
                }
            }
        }

        static Logger &instance()
        {
            static Logger logger;
            return logger;
        }

        void set_level(const std::string &level)
        {  
            min_level_.store(from_string(level), std::memory_order_release);
        }

        void set_formatter(std::unique_ptr<Formatter> fmt)
        {
            formatter_ = std::move(fmt);
        }

        std::unique_ptr<Formatter> &get_formatter()
        {
            return formatter_;
        }

        void set_transmitter(std::unique_ptr<Transmitter> transmitter)
        {
            transmitter_ = std::move(transmitter);
        }

        std::unique_ptr<Transmitter> &get_transmitter()
        {
            return transmitter_;
        }

        void add_sink(std::unique_ptr<Sink> sink)
        {
            sinks_.push_back(std::move(sink));
        }

        void clear_sinks()
        {
            sinks_.clear();
        }

        void set_danger_level(const std::string &level)
        {
            danger_level_ = from_string(level);
        }

        void operator()(const Level lv, const char *file, int line, const std::string content)
        {
            if (lv < min_level_.load(std::memory_order_acquire))
                return;
            LogMsg msg(lv, file, line, content);
            transmitter_->send(formatter_->format(msg), sinks_, lv < danger_level_);
        }

        LogStream operator()(const Level lv, const char *file, int line)
        {
            return LogStream(*this, lv, file, line, danger_level_);
        }

    private:
        Logger()
            : min_level_(Level::TRACE), formatter_(make_default_formatter()), transmitter_(make_async_transmitter()), exit_requested_(false)
        {
            
        }

        static void signal_handler(int sig)
        {
            // 强制刷日志
            instance().flush_all();

            // 恢复到默认行为，重新触发信号（让程序正常退出）
            signal(sig, SIG_DFL);
            raise(sig);
        }

    private:
        std::atomic<Level> min_level_;
        std::unique_ptr<Formatter> formatter_;
        std::vector<std::unique_ptr<Sink>> sinks_;
        std::unique_ptr<Transmitter> transmitter_;
        Level danger_level_ = Level::ERROR;

        std::atomic<bool> exit_requested_;
    };

    static Logger &logger = Logger::instance();

// 字符串版 —— 直接传内容
#define LOG_TRACE(content) logger(mylog::Level::TRACE, __FILE__, __LINE__, content)
#define LOG_DEBUG(content) logger(mylog::Level::DEBUG, __FILE__, __LINE__, content)
#define LOG_INFO(content) logger(mylog::Level::INFO, __FILE__, __LINE__, content)
#define LOG_WARN(content) logger(mylog::Level::WARN, __FILE__, __LINE__, content)
#define LOG_ERROR(content) logger(mylog::Level::ERROR, __FILE__, __LINE__, content)
#define LOG_FATAL(content) logger(mylog::Level::FATAL, __FILE__, __LINE__, content)

// 流式版 —— 不传 content，用 << 追加
#define LOG_TRACE_STREAM() logger(mylog::Level::TRACE, __FILE__, __LINE__)
#define LOG_DEBUG_STREAM() logger(mylog::Level::DEBUG, __FILE__, __LINE__)
#define LOG_INFO_STREAM() logger(mylog::Level::INFO, __FILE__, __LINE__)
#define LOG_WARN_STREAM() logger(mylog::Level::WARN, __FILE__, __LINE__)
#define LOG_ERROR_STREAM() logger(mylog::Level::ERROR, __FILE__, __LINE__)
#define LOG_FATAL_STREAM() logger(mylog::Level::FATAL, __FILE__, __LINE__)
} // namespace log
