// ChatSdk.hpp
#pragma once

#include "Common.hpp"
#include "APIConfig.hpp"
#include "LLMManager.hpp"
#include "SessionManager.hpp"
#include <memory>
#include <string>
#include <ctime>
#include <vector>

namespace ai_chat_sdk
{

    class ChatSDK
    {
    public:
        // 构造函数
        ChatSDK();
        // 初始化模型(必须提供配置好的模型配置 config)
        bool init_models(std::vector<std::shared_ptr<BaseConfig>> &configs);
        // 获取默认 configs (固定下标顺序为: [0]deepseek->[1]chatgpt->[2]gemini->[3]ollama, 除本地模型，其余需自己提供设置 key)
        std::vector<std::shared_ptr<BaseConfig>> get_default_configs();
        // 创建session
        std::string create_session(const std::string &model_name);
        // 获取指定完整会话
        std::shared_ptr<Session> get_session(const std::string &session_id);
        // 按时间降序获取所有会话列表 (会话 id)
        std::vector<std::string> get_session_list();
        // 删除会话
        bool delete_session(const std::string &session_id);
        // 获取可用模型列表
        std::vector<ModelInfo> get_available_models();
        // 发送消息 - 全量返回
        std::string send_message(const std::string session_id, const std::string &message);
        // 发送消息 - 流式响应
        std::string send_message_stream(const std::string session_id, const std::string &message, std::function<void(const std::string &, bool)> callback);

    private:
        // 注册所有模型
        void register_all_provider();

    private:
        bool initialized_;
        std::unordered_map<std::string, std::shared_ptr<BaseConfig>> configs_;
        std::unique_ptr<LLMManager> llm_manager_;
        std::unique_ptr<SessionManager> session_manager_;
    };
} // end ai_chat_sdk