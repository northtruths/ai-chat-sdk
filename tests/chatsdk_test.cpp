#include "ChatSDK.hpp"
#include "utils/logger.hpp"
#include "DeepSeekConfig.hpp"
#include "OllamaConfig.hpp"
#include <iostream>
#include <fstream>

using namespace ai_chat_sdk;

void set_log()
{
    mylog::Logger &logger = mylog::Logger::instance();
    logger.set_level(mylog::Level::TRACE);
    logger.set_formatter(mylog::make_default_formatter());
    logger.set_transmitter(mylog::make_async_transmitter(4 * 1024 * 1024, 1000));
    logger.add_sink(mylog::make_file_sink("../logs", "chat_sdk_test.log"));
    //logger.add_sink(mylog::make_console_sink());
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

    LOG_INFO("=== ChatSDK 测试开始 ===");

    // ============================================================
    //  1. 加载配置
    // ============================================================
    Json::Value env_config = load_config();
    if (env_config.isNull())
    {
        std::cout << "配置文件加载失败！" << std::endl;
        return -1;
    }

    std::string deepseek_key = env_config["deepseek_api_key"].asString();
    std::string gemini_key = env_config["gemini_api_key"].asString();

    // ============================================================
    //  2. 创建 ChatSDK
    // ============================================================
    ChatSDK chat_sdk;
    LOG_INFO("ChatSDK 创建成功");

    // ============================================================
    //  3. 准备模型配置
    // ============================================================
    auto h_configs = chat_sdk.get_default_configs();

    // 3.1 DeepSeek 配置
    h_configs["deepseek"]->set_api_key(deepseek_key);
    h_configs["gemini"]->set_api_key(gemini_key);

    std::vector<std::shared_ptr<BaseConfig>> configs;
    configs.push_back(h_configs["deepseek"]);
    LOG_INFO("DeepSeek 配置准备完成");
    configs.push_back(h_configs["gemini"]);
    LOG_INFO("gemini 配置准备完成");

    // ============================================================
    //  4. 初始化模型
    // ============================================================
    if (!chat_sdk.init_models(configs)) {
        LOG_ERROR("模型初始化失败");
        return -1;
    }
    LOG_INFO("模型初始化成功");

    // ============================================================
    //  5. 创建会话
    // ============================================================
    std::string session_id = chat_sdk.create_session("gemini");
    if (session_id.empty()) {
        LOG_ERROR("创建会话失败");
        return -1;
    }
    std::cout << " 会话创建成功: " << session_id << std::endl;

    // ============================================================
    //  6. 发送消息（全量）
    // ============================================================
    std::cout << "\n=== 发送消息（全量）===" << std::endl;
    std::string user_msg = "你好，我叫小木宁";
    std::cout << "用户: " << user_msg << std::endl;

    std::string reply = chat_sdk.send_message(session_id, user_msg);
    if (reply.empty()) {
        LOG_ERROR("发送消息失败");
    } else {
        std::cout << "AI: " << reply << std::endl;
        LOG_INFO("消息发送成功");
    }

    // ============================================================
    //  7. 发送第二条消息（验证上下文记忆）
    // ============================================================
    std::cout << "\n=== 发送消息（验证上下文）===" << std::endl;
    user_msg = "我叫什么名字？";
    std::cout << "用户: " << user_msg << std::endl;

    reply = chat_sdk.send_message(session_id, user_msg);
    if (reply.empty()) {
        LOG_ERROR("发送消息失败");
    } else {
        std::cout << "AI: " << reply << std::endl;
        LOG_INFO("消息发送成功");
    }

    // ============================================================
    //  8. 查看会话列表
    // ============================================================
    std::cout << "\n=== 会话列表 ===" << std::endl;
    auto sessions = chat_sdk.get_session_list();
    for (const auto& id : sessions) {
        auto session = chat_sdk.get_session(id);
        std::cout << "会话: " << id 
                  << ", 简介:" << session->session_desc_
                  << ", 模型: " << session->model_name_ 
                  << ", 消息数: " << session->messages_.size() << std::endl;
    }

    // ============================================================
    //  9. 查看历史消息
    // ============================================================
    std::cout << "\n=== 历史消息 ===" << std::endl;
    auto session = chat_sdk.get_session(session_id);
    if (session) {
        for (const auto& msg : session->messages_) {
            std::cout << msg.role_ << ": " << msg.content_ << std::endl;
        }
    }

    // ============================================================
    //  10. 获取可用模型列表
    // ============================================================
    std::cout << "\n=== 可用模型 ===" << std::endl;
    auto models = chat_sdk.get_available_models();
    for (const auto& model : models) {
        std::cout << "  - " << model << std::endl;
    }

    // ============================================================
    //  11. 删除会话
    // ============================================================
    // std::cout << "\n=== 删除会话 ===" << std::endl;
    // if (chat_sdk.delete_session(session_id)) {
    //     std::cout << "✅ 会话删除成功" << std::endl;
    // } else {
    //     std::cout << "❌ 会话删除失败" << std::endl;
    // }

    LOG_INFO("=== ChatSDK 测试结束 ===");
    return 0;
}