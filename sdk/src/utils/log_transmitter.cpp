#include "log_transmitter.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace mylog
{
    // 同步发送
    class SyncTransmitter : public Transmitter
    {
    public:
        void send(const std::string &formatted_msg,
                  std::vector<std::unique_ptr<Sink>> &sinks, bool is_safe) override
        {
            for (auto &sink : sinks)
            {
                sink->write(formatted_msg);
            }
            if(!is_safe){
                for(auto &sink : sinks){
                    sink->flush();
                }
            }
        }
    };

    std::unique_ptr<Transmitter> make_sync_transmitter()
    {
        return std::make_unique<SyncTransmitter>();
    }

    // 异步发送
    class AsyncTransmitter : public Transmitter
    {
    public:
        AsyncTransmitter(size_t buffer_size, int flush_interval_ms)
            : write_buff_(&buff_a_),
              flush_buff_(&buff_b_),
              buff_size_(buffer_size),
              is_running_(true),
              is_flush_(false),
              flush_interval_ms_(flush_interval_ms)
        {
            buff_a_.reserve(buff_size_);
            buff_b_.reserve(buff_size_);
            tflush_ = std::thread(&AsyncTransmitter::flush_loop, this);
        }
        ~AsyncTransmitter()
        {
            is_running_.store(false, std::memory_order_release);
            {
                std::unique_lock<std::mutex> lock(mtx_buff_);
                if (!write_buff_->empty())
                {
                    std::swap(write_buff_, flush_buff_);
                    is_flush_.store(true, std::memory_order_release);
                    cond_file_.notify_one();
                }
            }
            if (tflush_.joinable())
            {
                tflush_.join();
            }
        }

        void send(const std::string &formatted_msg,
                  std::vector<std::unique_ptr<Sink>> &sinks, bool is_safe) override
        {
            psinks_ = &sinks;
            std::lock_guard<std::mutex> lock(mtx_buff_);
            if (formatted_msg.size() + write_buff_->size() > buff_size_)
            {
                // 两个缓存都满了，直接丢弃
                if (is_flush_.load(std::memory_order_acquire))
                    return;
                flush();
            }
            *write_buff_ += formatted_msg;
            if(!is_safe){
                flush();
            }
        }

    private:
        void flush()
        {
            std::swap(write_buff_, flush_buff_);
            // 通知后台线程
            is_flush_.store(true, std::memory_order_release);
            cond_file_.notify_one();
        }

        void flush_loop()
        {
            while (true)
            {
                {
                    std::unique_lock<std::mutex> lock(mtx_file_);
                    cond_file_.wait_for(lock, std::chrono::milliseconds(flush_interval_ms_), [this]
                                        { return is_flush_.load(std::memory_order_acquire) || !is_running_.load(std::memory_order_acquire); });
                }

                if (!is_running_.load(std::memory_order_acquire) &&
                    !is_flush_.load(std::memory_order_acquire))
                    break;

                {
                    // 超时主动刷新
                    if (flush_buff_->empty() && !write_buff_->empty())
                    {
                        std::swap(write_buff_, flush_buff_);
                        is_flush_.store(true, std::memory_order_release);
                    }
                    else if (flush_buff_->empty())
                    {
                        continue;
                    }
                }

                // 安全地取出待刷数据和当时绑定的 sinks
                std::string data_to_flush;
                std::vector<std::unique_ptr<Sink>> *sinks_snapshot = nullptr;

                {
                    std::lock_guard<std::mutex> lock(mtx_buff_);
                    if (!flush_buff_->empty())
                    {
                        data_to_flush = std::move(*flush_buff_);
                        flush_buff_->clear();
                    }
                    // 记录此时与数据对应的 sinks
                    sinks_snapshot = psinks_;
                    is_flush_.store(false, std::memory_order_release);
                }

                if (!data_to_flush.empty() && sinks_snapshot)
                {
                    for (auto &sink : *sinks_snapshot)
                    {
                        sink->write(data_to_flush);
                        sink->flush();
                    }
                }
            }
        }
        // 缓冲区
        std::string buff_a_;
        std::string buff_b_;
        std::string *write_buff_;
        std::string *flush_buff_;
        size_t buff_size_;
        // 后台线程
        std::thread tflush_;
        std::mutex mtx_buff_;
        std::mutex mtx_file_;
        std::condition_variable cond_file_;
        std::atomic<bool> is_running_;
        std::atomic<bool> is_flush_;
        int flush_interval_ms_;
        // 发送信息
        std::vector<std::unique_ptr<Sink>> *psinks_;
    };

    std::unique_ptr<Transmitter> make_async_transmitter(size_t buffer_size, int flush_interval_ms)
    {
        return std::make_unique<AsyncTransmitter>(buffer_size, flush_interval_ms);
    }

}