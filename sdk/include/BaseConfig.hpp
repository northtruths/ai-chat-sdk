//模型参数配置抽象基类
#pragma once

#include "utils/logger.hpp"
#include "Session.hpp"
#include <jsoncpp/json/json.h>
#include <string>
#include <vector>

using namespace mylog;

namespace ai_chat_sdk
{
    class BaseConfig
    {
    public:
        virtual ~BaseConfig() = default;

        // 初始化默认配置
        virtual void init_defaults() = 0;

        // 设置模型完整URL
        virtual void set_url(const std::string &endpoint, const std::string &path) = 0;
        // 获取服务器地址
        virtual std::string get_endpoint() = 0;
        // 获取模型具体路径
        virtual std::string get_path() = 0;

        // 设置API_KEY
        virtual void set_api_key(const std::string &api_key) = 0;
        // 获取API_KEY
        virtual std::string get_api_key() = 0;

        // 通用接口, 支持任何 json 数据，但模型无对应参数则会无效
        virtual bool set(const std::string &key, const Json::Value &value) = 0;
        virtual Json::Value get(const std::string &key) const = 0;

        // 便捷化接口
        virtual void set_model(const std::string &model) = 0;
        virtual std::string get_model() const = 0;

        virtual void set_temperature(double temp) = 0;
        virtual double get_temperature() const = 0;

        virtual void set_max_tokens(int tokens) = 0;
        virtual int get_max_tokens() const = 0;

        virtual void set_stream(bool stream) = 0;
        virtual bool get_stream() const = 0;

        virtual void set_model_desc(const std::string &desc) = 0;
        virtual std::string get_model_desc() const = 0;

        // 消息管理
        virtual void set_messages(const std::vector<Message> &messages) = 0;
        virtual void clear_messages() = 0;
        virtual Json::Value get_messages() const = 0;

        // 获取完整Json Body
        virtual const Json::Value &asJson() const = 0;

        //获取大模型系列名称（非具体模型版本）
        virtual std::string get_series_name() const = 0;

    protected:
        // 请求头
        // 服务器地址
        std::string endpoint_;
        // api_path
        std::string api_path_;
        // aip_key
        std::string api_key_;

        // 请求body
        // Json数据
        Json::Value data_;

        // 模型描述
        std::string desc_;
    };
}
