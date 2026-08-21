#include "DeepSeekProvider.hpp"
#include "DeepSeekConfig.hpp"
#include "utils/logger.hpp"
#include <iostream>
#include <fstream>
#include <jsoncpp/json/json.h>

using namespace ai_chat_sdk;

void set_log()
{
    mylog::Logger &logger = mylog::Logger::instance();
    logger.set_level(mylog::Level::TRACE);
    logger.set_formatter(mylog::make_default_formatter());
    logger.set_transmitter(mylog::make_async_transmitter(4 * 1024 * 1024, 1000));
    logger.add_sink(mylog::make_file_sink("../logs", "stream_test.log"));
}

Json::Value load_config(const std::string &filename = "../config.json")
{
    Json::Value root;
    std::ifstream file(filename);

    if (!file.is_open())
    {
        LOG_WARN("未找到 config.json，使用默认值");
        return root;
    }

    Json::Reader reader;
    reader.parse(file, root);
    return root;
}

int main()
{
    set_log();

    DeepSeekConfig cf;

    Json::Value env_config = load_config();
    if (env_config.isNull())
    {
        std::cout << "配置文件加载失败！" << std::endl;
        return -1;
    }

    std::string endpoint = env_config["endpoint"].asString();
    std::string path = env_config["path"].asString();
    std::string key = env_config["api_key"].asString();
    cf.set_url(endpoint, path);
    cf.set_api_key(key);
    Json::Value thinking;
    thinking["type"] = "disabled";
    cf.set("thinking", thinking);
    cf.set_max_tokens(4096);
    cf.add_message("system", "你是 DeepSeek，由深度求索公司开发的 AI 助手。你应当友好、专业、清晰地回答用户问题。用户是对话的主体，你是辅助者。请根据上下文给出准确、有帮助的回答。");

    DeepSeekProvider deepseek(&cf);

    std::cout << "=== DeepSeek 流式对话测试 ===" << std::endl;
    std::cout << "输入消息，按 Enter 发送" << std::endl;
    std::cout << "输入 '退出！' 或 '退出!' 结束" << std::endl;

    while (true)
    {
        std::string cin_message;
        std::cout << "\n你: \n";
        std::getline(std::cin, cin_message);

        if (cin_message.empty())
        {
            continue;
        }

        if (cin_message == "退出！" || cin_message == "退出!")
        {
            std::cout << "结束对话，已退出" << std::endl;
            break;
        }

        std::cout << "\nAI: \n" << std::flush;

        std::string full_reply;
        auto write_chunk = [&](const std::string &chunk, bool last)
        {
            std::cout << chunk << std::flush;
            full_reply += chunk;
            if (last)
            {
                std::cout << std::endl;
            }
        };

        std::string resp = deepseek.send_message_stream(cin_message, write_chunk);

        if (resp.empty() && full_reply.empty())
        {
            std::cout << "发送失败！" << std::endl;
            return 0;
        }
    }

    return 0;
}