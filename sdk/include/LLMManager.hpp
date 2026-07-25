#pragma once

#include "Common.hpp"
#include <string>
#include <memory>

namespace ai_chat_sdk
{
    class Message;
    class LLMManager
    {
    public:
        virtual ~LLMManager() = default;
        // 初始化模型
        virtual bool init_model(const Config fg) = 0;
        // 检测模型是否有效
        virtual bool is_available() const = 0;
        // 全量式发送信息
        virtual std::string  send_message() = 0;
        // 流式发送信息
        virtual std::string  send_meesage_stream() = 0;
        // 获取模型名称
        virtual std::string get_model_name() const = 0;
        // 获取模型描述
        virtual std::string get_model_desc() const = 0;
        // 设置/更改配置参数
        virtual bool set_model_params(const Config cf) = 0;
        // 获取模型配置信息
        virtual Config &get_model_setting() const = 0;

    protected:
        // 模型是否有效（是否初始化）
        bool is_available_;
        // 模型的参数
        Config config_;
        // 模型的信息模块
        std::unique_ptr<Message> message_;
    };
}