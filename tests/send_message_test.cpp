// 测试deepseek全量返回

#include "DeepSeekProvider.hpp"
#include "utils/logger.hpp"
#include "DeepSeekConfig.hpp"
#include <memory>
#include <string>
#include <iostream>
#include <fstream>

using namespace ai_chat_sdk;

void set_log()
{
    mylog::Logger &logger = mylog::Logger::instance();
    logger.set_level(mylog::Level::TRACE);
    logger.set_formatter(mylog::make_default_formatter());
    logger.set_transmitter(mylog::make_async_transmitter(4 * 1024 * 1024, 1000));
    logger.add_sink(mylog::make_file_sink("../logs", "test.log"));
}

Json::Value load_config(const std::string &filename = "../config.json")
{
    Json::Value root;
    std::ifstream file(filename);

    if (!file.is_open())
    {
        LOG_WARN("未找到 config.json，模型使用默认值");
        return root;
    }

    Json::Reader reader;
    reader.parse(file, root);
    return root;
}

int main()
{
    // 配置日志
    set_log();

    // 大模型配置，URL和KEY没有默认配置，其他的都有默认配置
    DeepSeekConfig cf;
    std::string endpoint = "https://api.deepseek.com";
    std::string path = "/chat/completions";
    std::string key;
    // 读取本地配置
    Json::Value env_config = load_config();
    if (!env_config.isNull())
    {
        endpoint = env_config["endpoint"].asString();
        path = env_config["path"].asString();
        key = env_config["api_key"].asString();
        cf.set_url(endpoint, path);
        cf.set_api_key(key);
    }
    else
    {
        std::cout << "配置文件加载失败！" << std::endl;
    }

    // 创建模型
    DeepSeekProvider deepseek(&cf);

    while (true)
    {
        std::string cin_message;
        std::cin >> cin_message;
        if(cin_message == "退出！" || cin_message == "退出!"){
            std::cout << "结束对话，已退出" << std::endl;
            break;
        }
        // 发送信息
        std::string resp = deepseek.send_message(cin_message);
        if (resp.empty())
        {
            std::cout << "发送失败！" << std::endl;
        }
        else
        {
            std::cout << resp << std::endl;
        }
    }
    return 0;
}