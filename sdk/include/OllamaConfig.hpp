//模型参数配置Ollama类
#pragma once

#include "BaseConfig.hpp"
#include <jsoncpp/json/json.h>
#include <string>
#include <iostream>

namespace ai_chat_sdk
{
    class OllamaConfig : public BaseConfig
    {
    public:
        // 构造函数
        OllamaConfig();

        // 初始化默认设置
        void init_defaults();

        // 设置模型完整URL
        void set_url(const std::string &endpoint, const std::string &path);
        // 获取服务器地址
        std::string get_endpoint();
        // 获取模型具体路径
        std::string get_path();

        // 设置API_KEY，ollama不需要所以空处理
        void set_api_key(const std::string &api_key){(void)api_key;};
        // 获取API_KEY
        std::string get_api_key(){return std::string();};

        // 通用设置接口
        bool set(const std::string &key, const Json::Value &value);
        Json::Value get(const std::string &key) const;

        // 便捷化接口
        void set_model(const std::string &model);
        std::string get_model() const;

        void set_temperature(double temp);
        double get_temperature() const;

        void set_max_tokens(int tokens);
        int get_max_tokens() const;

        void set_stream(bool stream);
        bool get_stream() const;

        void set_model_desc(const std::string &desc);
        std::string get_model_desc() const;

        // 消息管理
        void set_messages(const std::vector<Message> &messages);
        void clear_messages();
        Json::Value get_messages() const;

        // 返回整个Json
        const Json::Value &asJson() const;
        
        std::string get_series_name() const;
    };
}