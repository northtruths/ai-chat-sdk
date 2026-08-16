// log_sink.hpp
#pragma once
#include <string>
#include <memory>
#include <fstream>
#include <ctime>

namespace mylog
{
    // ======= 抽象基类 =======
    class Sink
    {
    public:
        //析构函数
        virtual ~Sink() = default;

        //日志写入目标地点
        virtual void write(const std::string &formatted_msg) = 0;

        //日志刷新落地
        virtual void flush() = 0;
    };


    // ======= 落地控制台 =======
    class ConsoleSink : public Sink
    {
    public:
        ~ConsoleSink();
        void write(const std::string &formatted_msg) override;
        void flush() override;
    };

    // ======= 落地文件 =======
    class FileSink : public Sink
    {
    public:
        FileSink(const std::string &dir, const std::string &filename);
        ~FileSink();
        void write(const std::string &formatted_msg) override;
        void flush() override;

    private:
        void open();

        std::string dir_;
        std::string filename_;
        std::ofstream file_;
    };


    // ======= 滚动文件落地 =======
    class RollFileSink : public Sink
    {
    public:
        RollFileSink(const std::string &dir,
                     const std::string &base_name,
                     size_t max_size);
        void write(const std::string &formatted_msg) override;
        void flush() override;

    private:
        void make_time_dir(std::tm *tm_buf);
        void roll();
        std::string make_path();

    private:
        std::string dir_;
        std::string base_name_;
        size_t max_size_;
        std::ofstream file_;
        size_t cur_written_ = 0;
        std::time_t time_last_ = 0;
        std::string time_dir_;
        int index_ = 0;
    };

    // 工厂函数
    // 落地在控制台
    std::unique_ptr<Sink> make_console_sink();

    // 落地在文件
    std::unique_ptr<Sink> make_file_sink(const std::string &dir = "./logs", const std::string &filename = "app.log");
    std::unique_ptr<Sink> make_file_sink(const char *dir, const char *filename);

    // 文件滚动落地
    std::unique_ptr<Sink> make_roll_file_sink(
        const std::string &dir = "./logs",
        const std::string &base_name = "app.log",
        size_t max_size = 10 * 1024 * 1024);
} // namespace log