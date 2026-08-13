#pragma once

#include "LLMProvider.hpp"
#include "Session.hpp"
#include "BaseConfig.hpp"

namespace ai_chat_sdk
{
    class LLMManager
    {
    public:
        LLMManager() = default; 
        
        // 注册 Provider
        bool register_provider(const std::string &name, std::unique_ptr<LLMProvider> provider);

        // 获取 Provider
        LLMProvider *get_provider(const std::string &name) const;

        // 发送消息（非流式）
        std::string send_message(const std::string &modelName,
                                const std::vector<Message> &messages);

        // 发送消息（流式）
        std::string send_message_stream(const std::string &modelName,
                                      const std::vector<Message> &messages,
                                      std::function<void(const std::string &, bool)> callback);

        // 获取可用模型
        std::vector<std::string> get_available_models() const;

        // 检查模型是否可用
        bool is_model_available(const std::string &name) const;

        // 获取所有已注册模型模型
        std::vector<std::string> get_registed_models() const;

    private:
        std::unordered_map<std::string, std::unique_ptr<LLMProvider>> providers_;
    };
}