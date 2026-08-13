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

    // 设置模型完整URL
    void DeepSeekConfig::set_url(const std::string &endpoint, const std::string &path)
    {
        endpoint_ = endpoint;
        api_path_ = path;
    }
    // 获取服务器地址
    std::string DeepSeekConfig::get_endpoint()
    {
        return endpoint_;
    }
    // 获取模型具体路径
    std::string DeepSeekConfig::get_path()
    {
        return api_path_;
    }

    // 设置API_KEY
    void DeepSeekConfig::set_api_key(const std::string &api_key)
    {
        api_key_ = api_key;
    }
    // 获取API_KEY
    std::string DeepSeekConfig::get_api_key()
    {
        return api_key_;
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

    void DeepSeekConfig::set_model_desc(const std::string &desc = "DeepSeek，高效能AI助手，具备深度语义理解、多领域知识覆盖与长文本处理能力，且完全免费，模型持续迭代升级。")
    {
        desc_ = desc;
    }

    std::string DeepSeekConfig::get_model_desc() const
    {
        return desc_;
    }

    // 消息管理
    void DeepSeekConfig::set_messages(const std::vector<Message> &messages)
    {
        //每次发送信息都是全部发送，因此要清空之前的记录避免重复
        clear_messages();

        for (auto &message : messages)
        {
            Json::Value msg;
            msg["role"] = message.role_;
            msg["content"] = message.content_;
            data_["messages"].append(msg);
        }
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

    std::string DeepSeekConfig::get_series_name(){
        return "deepseek";
    }

}
