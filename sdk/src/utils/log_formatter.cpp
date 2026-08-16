// log_formatter.cpp
#include "log_formatter.hpp"
#include "log_message.hpp"
#include "log_level.hpp"
#include <string>
#include <ctime>
#include <memory>

namespace mylog
{

    class DefaultFormatter : public Formatter
    {
    public:
        std::string format(LogMsg &msg) override
        {
            std::string temp;

            // [级别]
            temp += "[";
            temp += level_to_string(msg.level_);
            temp += "] ";

            // [时间]
            temp += "[";
            temp += format_time(msg.timestamp_);
            temp += "] ";

            // [文件名]
            temp += "[ ";
            temp += msg.file_;
            temp += " ] ";
            // [行号]
            temp += "[";
            temp += std::to_string(msg.line_);
            temp += "] ";
            // [线程号]
            temp += "[pid ";
            temp += std::to_string(std::hash<std::thread::id>{}(msg.tid_));
            temp += "] ";
            // 正文
            temp += " - ";
            temp += msg.content_;
            temp += "\n";

            return temp;
        }

    private:
        std::string format_time(const std::chrono::system_clock::time_point &tp)
        {
            auto t = std::chrono::system_clock::to_time_t(tp);
            std::tm tm_buf;
            localtime_r(&t, &tm_buf);

            // 预分配 20 字节（YYYY-MM-DD HH:MM:SS 正好 19 位 + '\0'）
            std::string result;
            result.reserve(20);

            auto append_two_digits = [&result](int value)
            {
                result.push_back('0' + value / 10);
                result.push_back('0' + value % 10);
            };

            // 年（4 位）
            int year = tm_buf.tm_year + 1900;
            result.push_back('0' + year / 1000);
            result.push_back('0' + (year / 100) % 10);
            result.push_back('0' + (year / 10) % 10);
            result.push_back('0' + year % 10);

            result.push_back('-');
            append_two_digits(tm_buf.tm_mon + 1);
            result.push_back('-');
            append_two_digits(tm_buf.tm_mday);
            result.push_back(' ');
            append_two_digits(tm_buf.tm_hour);
            result.push_back(':');
            append_two_digits(tm_buf.tm_min);
            result.push_back(':');
            append_two_digits(tm_buf.tm_sec);

            return result;
        }
    };

    // 工厂函数
    std::unique_ptr<Formatter> make_default_formatter()
    {
        return std::make_unique<DefaultFormatter>();
    }

} // namespace log