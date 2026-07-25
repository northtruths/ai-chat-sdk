#pragma once

#include "LLMManager.hpp"

namespace ai_chat_sdk
{
    class DeepSeekProvider : LLMManager
    {
    public:
        // 初始化模型
        bool init_model(const Config cf);
        // 检测模型是否有效
        bool is_available() const;
        // 全量式发送信息
        std::string  send_message();
        // 流式发送信息
        std::string  send_meesage_stream();
        // 获取模型名称
        std::string get_model_name() const;
        // 获取模型描述
        std::string get_model_desc() const;
        // 设置/更改配置参数
        bool set_model_params(const Config cf);
        // 获取模型配置信息
        Config &get_model_setting();
    private:
        //初始化默认设置
        void init_config(){
            
        }

    private:
        std::string name_;
        std::string endpoint_;
        std::string api_key_;
    };
}
