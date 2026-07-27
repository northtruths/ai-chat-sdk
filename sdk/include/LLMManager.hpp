#pragma once

#include "Common.hpp"
#include <BaseConfig.hpp>
#include <string>
#include <memory>
#include <jsoncpp/json/json.h>

namespace ai_chat_sdk
{
    class Message;
    class LLMManager
    {
    public:
        virtual ~LLMManager() = default;
        // 初始化模型
        virtual bool init_model(BaseConfig *cf) = 0;
        // 检测模型是否有效
        virtual bool is_available() const = 0;
        // 全量式发送信息
        virtual std::string send_message(const std::string content) = 0;
        // 流式发送信息
        virtual std::string send_meesage_stream(const std::string content) = 0;
        // 获取模型名称
        virtual std::string get_model() const = 0;
        // 获取模型描述
        virtual std::string get_model_desc() const = 0;
        // 设置/更改配置参数
        virtual bool set_params(const std::string &key, const Json::Value &value) = 0;
        // 获取模型配置信息
        virtual Json::Value get_params(const std::string &key) const = 0;

    protected:
        // 模型是否有效（是否初始化）
        bool is_available_ = false;
        // 模型的参数
        BaseConfig *config_;
    };
}