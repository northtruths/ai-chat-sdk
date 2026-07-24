#pragma once

#include <string>
#include <unordered_map>
#include <memory>

namespace ai_chat_sdk{
    class Message;
    class LLMManager{
        public:
            virtual ~LLMManager() = default;
            //初始化模型
            virtual bool init_model(std::unordered_map<std::string, std::string>) = 0;
            //检测模型是否有效
            virtual bool is_available() const = 0;
            //全量式发送信息
            virtual void send_message() = 0;
            //流式发送信息
            virtual void send_meesage_stream() = 0;
            //获取模型名称
            virtual std::string get_model_name() const = 0;
            //获取模型描述
            virtual std::string get_model_desc() const = 0;
        private:
            //模型是否有效（是否初始化）
            bool is_available_;
            //模型名称
            std::string name_;
            //模型的位置（服务器地址）
            std::string endpoint_;
            //模型的其余参数
            std::string config_;
            //模型的信息模块
            std::unique_ptr<Message> message_;
    };
}