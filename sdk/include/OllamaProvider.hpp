#pragma once

#include "LLMProvider.hpp"
#include <memory>

namespace ai_chat_sdk
{    
    class OllamaProvider : public LLMProvider
    {
    public:
        // 构造函数，可以构造时传配置直接初始化
        OllamaProvider() = default;
        OllamaProvider(std::shared_ptr<BaseConfig> cf);
        // 设置模型配置
        bool set_model(std::shared_ptr<BaseConfig> cf);
        // 检测模型是否有效
        bool is_available() const;
        // 全量式发送信息
        std::string send_message(const std::vector<Message> &messages);
        // 流式发送信息
        std::string send_message_stream(const std::vector<Message> &messages, one_chunk callback);
        // 获取模型名称
        std::string get_model() const;
        // 获取模型描述
        std::string get_model_desc() const;
        // 获取模型参数信息
        Json::Value get_params(const std::string &key) const;
    };
}
