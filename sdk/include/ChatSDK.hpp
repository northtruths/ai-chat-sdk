// ChatSdk.hpp
#pragma once

#include "BaseConfig.hpp"
#include "OllamaConfig.hpp"
#include "DeepSeekConfig.hpp"
#include "LLMManager.hpp"
#include "Common.hpp"
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
        // 初始化模型
        bool init_models(std::vector<std::shared_ptr<BaseConfig>> &configs);
        // 创建session
        std::string create_session(const std::string &model_name);
        // 获取会话
        std::shared_ptr<Session> get_session(const std::string &session_id);
        // 获取所有会话列表
        std::vector<std::string> get_session_list();//加上desc
        // 删除会话
        bool delete_session(const std::string &session_id);
        // 获取可⽤模型列表
        std::vector<std::string> get_available_models();
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