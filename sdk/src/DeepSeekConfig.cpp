#include "DeepSeekConfig.hpp"

namespace ai_chat_sdk
{
    // 构造函数
    DeepSeekConfig::DeepSeekConfig()
    {
        init_defaults();
    }
    void DeepSeekConfig::init_defaults()
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

    // 通用设置接口
    bool DeepSeekConfig::set(const std::string &key, const Json::Value &value)
    {
        if (data_.isMember(key))
        {
            data_[key] = value;
            return true;
        }
        LOG_WARN_STREAM() << "参数设置失败：不支持 " << key;
        return false;
    }

    Json::Value DeepSeekConfig::get(const std::string &key) const
    {
        if (data_.isMember(key))
        {
            return data_[key];
        }
        return Json::Value::null;
    }

    // 便捷化接口
    void DeepSeekConfig::set_model(const std::string &model)
    {
        set("model", model);
    }
    std::string DeepSeekConfig::get_model() const
    {
        return get("model").asString();
    }

    void DeepSeekConfig::set_temperature(double temp)
    {
        set("temperature", temp);
    }
    double DeepSeekConfig::get_temperature() const
    {
        return get("temperature").asDouble();
    }

    void DeepSeekConfig::set_max_tokens(int tokens)
    {
        set("max_tokens", tokens);
    }
    int DeepSeekConfig::get_max_tokens() const
    {
        return get("max_tokens").asInt();
    }

    void DeepSeekConfig::set_stream(bool stream)
    {
        set("stream", stream);
    }
    bool DeepSeekConfig::get_stream() const
    {
        return get("stream").asBool();
    }

    void DeepSeekConfig::set_model_desc(const std::string &desc)
    {
        desc_ = desc;
    }
    std::string DeepSeekConfig::get_model_desc() const
    {
        return desc_;
    }

    // 消息管理
    void DeepSeekConfig::add_message(const std::string &role, const std::string &content)
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

    void DeepSeekConfig::clear_messages()
    {
        data_["messages"] = Json::Value(Json::arrayValue);
    }

    Json::Value DeepSeekConfig::get_messages() const
    {
        return get("messages");
    }

    // 返回整个Json
    const Json::Value &DeepSeekConfig::asJson() const
    {
        return data_;
    }

    // 打印调试
    void DeepSeekConfig::print_all() const
    {
        auto keys = data_.getMemberNames();
        for (const auto &key : keys)
        {
            const auto &value = data_[key];
            std::cout << key << " = " << value.toStyledString() << std::endl;
        }
    }
}
