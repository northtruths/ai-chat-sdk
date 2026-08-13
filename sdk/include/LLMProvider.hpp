#pragma once

#include "Common.hpp"
#include <BaseConfig.hpp>
#include <string>
#include <memory>
#include <jsoncpp/json/json.h>
#include <functional>

namespace ai_chat_sdk
{
    using one_chunk = std::function<void(const std::string &, bool)>;

    class LLMProvider
    {
    public:
        virtual ~LLMProvider() = default;
        // 设置模型配置
        virtual bool set_model(std::shared_ptr<BaseConfig> &cf) = 0;
        // 检测模型是否有效
        virtual bool is_available() const = 0;
        // 全量式发送信息
        virtual std::string send_message(const std::vector<Message> &messages) = 0;
        // 流式发送信息
        virtual std::string send_message_stream(const std::vector<Message> &messages, one_chunk callback) = 0;
        // 获取模型名称
        virtual std::string get_model() const = 0;
        // 获取模型描述
        virtual std::string get_model_desc() const = 0;
        // 获取模型参数信息
        virtual Json::Value get_params(const std::string &key) const = 0;

    protected:
        // 模型是否有效（是否初始化）
        bool is_available_ = false; 
        // 模型配置
        std::shared_ptr<BaseConfig> config_;
    };
}