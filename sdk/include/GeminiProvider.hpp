#pragma once

#include "LLMProvider.hpp"

namespace ai_chat_sdk
{    
    class GeminiProvider : public LLMManager
    {
    public:
        // 构造函数，可以构造时传配置直接初始化
        GeminiProvider() = default;
        GeminiProvider(BaseConfig *cf);
        // 初始化模型
        bool init_model(BaseConfig *cf);
        // 检测模型是否有效
        bool is_available() const;
        // 全量式发送信息
        std::string send_message(const std::string content);
        // 流式发送信息
        std::string send_message_stream(const std::string content, one_chunk callback);
        // 获取模型名称
        std::string get_model() const;
        // 获取模型描述
        std::string get_model_desc() const;
        // 更改配置参数
        bool set_params(const std::string &key, const Json::Value &value);
        // 获取模型配置信息
        Json::Value get_params(const std::string &key) const;
    };
}
