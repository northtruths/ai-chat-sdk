#include "ChatServer.hpp"
#include <gflags/gflags.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <ai_chat_sdk/utils/logger.hpp>

// 定义gflags参数
DEFINE_string(host, "0.0.0.0", "服务器绑定的地址");
DEFINE_int32(port, 8080, "服务器绑定的端口号");
DEFINE_string(log_level, "INFO", "日志级别");
DEFINE_double(temperature, 0.7, "温度值，影响生成文本的随机性");
DEFINE_int32(max_tokens, 2048, "最大token数");
DEFINE_string(config_file, "ChatServer.conf", "服务器配置文件路径");
DEFINE_string(key_file, "Key.json", "API密钥配置文件路径");
DEFINE_string(db_path, "chat.db", "数据库文件路径");
DEFINE_string(ollama_model_name, "", "Ollama模型名称");
DEFINE_string(ollama_model_desc, "", "Ollama模型描述");
DEFINE_string(ollama_endpoint, "", "Ollama 服务器地址");
DEFINE_string(ollama_path, "/api/chat", "Ollama API路径");

const std::string version = "1.0.0";

// 从环境变量获取API密钥
Json::Value get_env_var(const std::string &key)
{
    char *value = std::getenv(key.c_str());
    return value ? std::string(value) : "";
}

// 加载密钥配置文件（从文件中获取API密钥）
Json::Value load_config(const std::string &filename)
{
    Json::Value root;
    std::ifstream file(filename);

    if (!file.is_open())
    {
        LOG_WARN("未找到密钥配置文件");
        return root;
    }

    Json::Reader reader;
    reader.parse(file, root);
    return root;
}

// 验证配置参数
bool validate_config(ai_chat_server::ServerConfig &config)
{
    if (config.temperature_ < 0.0 || config.temperature_ > 2.0)
    {
        LOG_ERROR_STREAM() << "错误: 温度值必须在0.0到2.0之间，当前值: " << config.temperature_;
        return false;
    }

    if (config.max_tokens_ <= 0)
    {
        LOG_ERROR_STREAM() << "错误: 最大token数必须为正数，当前值: " << config.max_tokens_;
        return false;
    }

    bool has_cloud_api_key = !config.deepseek_api_key_.empty() ||
                             !config.chatgpt_api_key_.empty() ||
                             !config.gemini_api_key_.empty();
    bool has_ollama_config = !config.ollama_model_name_.empty() &&
                             !config.ollama_endpoint_.empty();
    if (!has_cloud_api_key && !has_ollama_config)
    {
        LOG_ERROR("错误: 至少需要提供一个有效的API密钥或Ollama模型配置");
        return false;
    }

    if (!config.ollama_model_name_.empty())
    {
        if (config.ollama_model_desc_.empty() || config.ollama_endpoint_.empty())
        {
            LOG_ERROR("错误: 如果提供了Ollama模型名称，则必须同时提供模型描述和端点");
            return false;
        }
    }

    return true;
}

// 显示接口说明
void show_api_info()
{
    std::cout << "\nChatServer API接口说明:\n";
    std::cout << "  POST   /api/session              - 创建新会话\n";
    std::cout << "  GET    /api/sessions             - 获取所有会话列表\n";
    std::cout << "  GET    /api/models               - 获取可用模型列表\n";
    std::cout << "  DELETE /api/session/{session_id} - 删除指定会话\n";
    std::cout << "  GET    /api/session/{session_id}/history - 获取会话历史消息\n";
    std::cout << "  POST   /api/message              - 发送消息(全量返回)\n";
    std::cout << "  POST   /api/message/async        - 发送消息(流式返回)\n";
    std::cout << "\n使用示例:\n";
    std::cout << "  # 基本启动\n";
    std::cout << "  ./AIChatServer\n";
    std::cout << "\n  # 指定端口启动\n";
    std::cout << "  ./AIChatServer --port=9000\n";
    std::cout << "\n  # 使用指定配置文件\n";
    std::cout << "  ./AIChatServer --config_file=my_config.conf\n";
    std::cout << "\n  # 设置环境变量后启动\n";
    std::cout << "  export DEEPSEEK_API_KEY=your_api_key\n";
    std::cout << "  ./AIChatServer\n";
}

// 日志设置
void set_log()
{
    mylog::Logger &logger = mylog::Logger::instance();
    logger.set_level(FLAGS_log_level);
    logger.set_formatter(mylog::make_default_formatter());
    logger.set_transmitter(mylog::make_async_transmitter(4 * 1024 * 1024, 1000));
    logger.add_sink(mylog::make_file_sink("./logs", "AIChatServer.log"));
    // logger.add_sink(mylog::make_console_sink());
}

int main(int argc, char *argv[])
{
    try
    {
        if (argc == 2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help"))
        {
            show_api_info();
            return 0;
        }

        // 设置使用说明
        gflags::SetUsageMessage("AIChatServer - AI聊天服务器\n\n使用方法: ./AIChatServer [options]");
        gflags::SetVersionString(version);

        // 先解析命令行获取配置文件路径，再加载配置文件，最后重新解析命令行
        // 以保证命令行参数优先于配置文件。
        gflags::ParseCommandLineFlags(&argc, &argv, false);
        if (!gflags::ReadFromFlagsFile(FLAGS_config_file, argv[0], false))
        {
            std::cerr << "无法读取配置文件: " << FLAGS_config_file << std::endl;
        }
        gflags::ParseCommandLineFlags(&argc, &argv, true);

        // 设置日志
        set_log();

        // 获取API密钥配置
        auto key_config = load_config(FLAGS_key_file);
        // 构建ServerConfig
        ai_chat_server::ServerConfig config;
        config.host_ = FLAGS_host;
        config.port_ = FLAGS_port;
        config.log_level_ = FLAGS_log_level;
        config.temperature_ = FLAGS_temperature;
        config.max_tokens_ = FLAGS_max_tokens;

        // 从文件配置获取API密钥
        config.deepseek_api_key_ = key_config["deepseek_api_key"].asString();
        config.chatgpt_api_key_ = key_config["chatgpt_api_key"].asString();
        config.gemini_api_key_ = key_config["gemini_api_key"].asString();

        // 从命令行参数获取Ollama配置
        config.ollama_model_name_ = FLAGS_ollama_model_name;
        config.ollama_model_desc_ = FLAGS_ollama_model_desc;
        config.ollama_endpoint_ = FLAGS_ollama_endpoint;
        config.ollama_path_ = FLAGS_ollama_path;

        // 验证配置参数
        if (!validate_config(config))
        {
            LOG_ERROR("配置验证失败，请检查参数设置");
            return 1;
        }

        // 显示当前配置
        LOG_INFO("AIChatServer 启动配置:");
        LOG_INFO_STREAM() << "  版本: " << version;
        LOG_INFO_STREAM() << "  主机: " << config.host_;
        LOG_INFO_STREAM() << "  端口: " << config.port_;
        LOG_INFO_STREAM() << "  日志级别: " << config.log_level_;
        LOG_INFO_STREAM() << "  温度值: " << config.temperature_;
        LOG_INFO_STREAM() << "  最大Token: " << config.max_tokens_;
        LOG_INFO_STREAM() << "  DeepSeek API Key: " << (config.deepseek_api_key_.empty() ? "未设置" : "已设置");
        LOG_INFO_STREAM() << "  ChatGPT API Key: " << (config.chatgpt_api_key_.empty() ? "未设置" : "已设置");
        LOG_INFO_STREAM() << "  Gemini API Key: " << (config.gemini_api_key_.empty() ? "未设置" : "已设置");
        LOG_INFO_STREAM() << "  Ollama 模型: " << (config.ollama_model_name_.empty() ? "未设置" : config.ollama_model_name_);
        LOG_INFO_STREAM() << "  Ollama 模型描述: " << (config.ollama_model_desc_.empty() ? "未设置" : config.ollama_model_desc_);
        LOG_INFO_STREAM() << "  Ollama 端点: " << (config.ollama_endpoint_.empty() ? "未设置" : config.ollama_endpoint_);
        // 启动时标准输出也写一份
        std::cout << "AIChatServer 启动配置:" << std::endl;
        std::cout << "  版本: " << version << std::endl;
        std::cout << "  主机: " << config.host_ << std::endl;
        std::cout << "  端口: " << config.port_ << std::endl;
        std::cout << "  日志级别: " << config.log_level_ << std::endl;
        std::cout << "  温度值: " << config.temperature_ << std::endl;
        std::cout << "  最大Token: " << config.max_tokens_ << std::endl;
        std::cout << "  DeepSeek API Key: " << (config.deepseek_api_key_.empty() ? "未设置" : "已设置") << std::endl;
        std::cout << "  ChatGPT API Key: " << (config.chatgpt_api_key_.empty() ? "未设置" : "已设置") << std::endl;
        std::cout << "  Gemini API Key: " << (config.gemini_api_key_.empty() ? "未设置" : "已设置") << std::endl;
        std::cout << "  Ollama 模型: " << (config.ollama_model_name_.empty() ? "未设置" : config.ollama_model_name_) << std::endl;
        std::cout << "  Ollama 模型描述: " << (config.ollama_model_desc_.empty() ? "未设置" : config.ollama_model_desc_) << std::endl;
        std::cout << "  Ollama 端点: " << (config.ollama_endpoint_.empty() ? "未设置" : config.ollama_endpoint_) << std::endl;

        // 创建并启动ChatServer
        ai_chat_server::ChatServer server(config);
        if (server.start())
        {
            LOG_INFO("ChatServer 启动成功!");
            LOG_INFO_STREAM() << "服务器地址: http://" << config.host_ << ":" << config.port_;
            std::cout << "ChatServer 启动成功!" << std::endl;
            std::cout << "服务器地址: http://" << config.host_ << ":" << config.port_ << std::endl;
            
            // 每隔段时间检查服务器是否在运行
            while (server.is_running())
            {
                std::this_thread::sleep_for(std::chrono::seconds(100));
            }
        }
        else
        {
            LOG_ERROR("ChatServer 启动失败!");
            return 1;
        }

        return 0;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR_STREAM() << "发生异常: " << e.what();
        return 1;
    }
    catch (...)
    {
        LOG_ERROR("发生未知异常");
        return 1;
    }
}