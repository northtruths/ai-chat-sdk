#pragma once

#include "utils/logger.hpp"
#include <jsoncpp/json/json.h>
#include <string>
#include <map>
#include <iostream>

using namespace log;

class DeepSeekConfig
{
public:
    // 构造函数：初始化默认配置
    DeepSeekConfig()
    {
        // 模型
        data_["model"] = "deepseek-v4-pro";

        // 思考模式
        data_["thinking"]["type"] = "enabled";
        data_["reasoning_effort"] = "high";

        // 生成控制
        data_["max_tokens"] = 4096;
        data_["temperature"] = 1.0;
        data_["top_p"] = 1.0;

        // 响应格式
        data_["response_format"]["type"] = "text";

        // 流式
        data_["stream"] = false;
        data_["stream_options"] = Json::Value::null;

        // 停止词
        data_["stop"] = Json::Value::null;

        // 工具调用
        data_["tools"] = Json::Value::null;
        data_["tool_choice"] = "none";

        // 日志
        data_["logprobs"] = false;
        data_["top_logprobs"] = Json::Value::null;

        // 消息数组（空）
        data_["messages"] = Json::Value(Json::arrayValue);
    }

    //通用设置接口
    bool set(const std::string &key, const Json::Value &value)
    {
        if (data_.isMember(key))
        {
            data_[key] = value;
            return true;
        }
        LOG_WARN_STREAM() "参数设置失败：不支持 " << key; 
        return false;
    }

    Json::Value get(const std::string &key) const
    {
        if (data_.isMember(key))
        {
            return data_[key]; 
        }
        return Json::Value::null;
    }

    //
    void set_model(const std::string &model) { set("model", model); }
    std::string get_model() const { return get("model").asString(); }

    void set_temperature(double temp) { set("temperature", temp); }
    double get_temperature() const { return get("temperature").asDouble(); }

    void set_max_tokens(int tokens) { set("max_tokens", tokens); }
    int get_max_tokens() const { return get("max_tokens").asInt(); }

    void set_stream(bool stream) { set("stream", stream); }
    bool get_stream() const { return get("stream").asBool(); }

    // ========== 消息管理 ==========
    void add_message(const std::string &role, const std::string &content)
    {
        if (!data_.isMember("messages") || !data_["messages"].isArray())
        {
            data_["messages"] = Json::Value(Json::arrayValue);
        }
        Json::Value msg;
        msg["role"] = role;
        msg["content"] = content;
        data_["messages"].append(msg);
    }

    void clear_messages()
    {
        data_["messages"] = Json::Value(Json::arrayValue);
    }

    Json::Value get_messages() const
    {
        return get("messages");
    }

    // ========== 输出整个 JSON ==========
    const Json::Value &asJson() const
    {
        return data_;
    }

    // ========== 打印调试 ==========
    void print_all() const
    {
        for (const auto &[key, value] : data_)
        {
            std::cout << key << " = " << value.toStyledString() << std::endl;
        }
    }

private:
    Json::Value data_; // 所有配置都存在这里
};