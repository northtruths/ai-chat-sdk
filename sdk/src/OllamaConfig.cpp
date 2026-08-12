#include "OllamaConfig.hpp"

namespace ai_chat_sdk
{
    // 构造函数
    OllamaConfig::OllamaConfig()
    {
        init_defaults();
    }
    void OllamaConfig::init_defaults()
    {
        // 模型名
        data_["model"] = "deepseek-r1:1.5b";

        // data_["think"] = false;

        // 生成控制
        data_["temperature"] = 0.7;
        data_["num_ctx"] = 2048;
        data_["stream"] = false;

        // 消息数组（空）
        data_["messages"] = Json::Value(Json::arrayValue);
    }
    // 设置模型完整URL
    void OllamaConfig::set_url(const std::string &endpoint, const std::string &path)
    {
        endpoint_ = endpoint;
        api_path_ = path;
    }
    // 获取服务器地址
    std::string OllamaConfig::get_endpoint()
    {
        return endpoint_;
    }
    // 获取模型具体路径
    std::string OllamaConfig::get_path()
    {
        return api_path_;
    }

    // 通用设置接口
    bool OllamaConfig::set(const std::string &key, const Json::Value &value)
    {
        if (data_.isMember(key))
        {
            data_[key] = value;
            return true;
        }
        LOG_WARN_STREAM() << "参数设置失败：不支持 " << key;
        return false;
    }

    Json::Value OllamaConfig::get(const std::string &key) const
    {
        if (data_.isMember(key))
        {
            return data_[key];
        }
        return Json::Value::null;
    }

    // 便捷化接口
    void OllamaConfig::set_model(const std::string &model)
    {
        set("model", model);
    }
    std::string OllamaConfig::get_model() const
    {
        return get("model").asString();
    }

    void OllamaConfig::set_temperature(double temp)
    {
        set("temperature", temp);
    }
    double OllamaConfig::get_temperature() const
    {
        return get("temperature").asDouble();
    }

    void OllamaConfig::set_max_tokens(int tokens)
    {
        set("num_ctx", tokens);
    }
    int OllamaConfig::get_max_tokens() const
    {
        return get("max_tokens").asInt();
    }

    void OllamaConfig::set_stream(bool stream)
    {
        set("stream", stream);
    }
    bool OllamaConfig::get_stream() const
    {
        return get("stream").asBool();
    }

    void OllamaConfig::set_model_desc(const std::string &desc = "Ollama：本地运行开源大模型，轻量免费。")
    {
        desc_ = desc;
    }

    std::string OllamaConfig::get_model_desc() const
    {
        return desc_;
    }

    // 消息管理
    void OllamaConfig::set_messages(const std::vector<Message> &messages)
    {
        clear_messages();
        for (auto &message : messages)
        {
            Json::Value msg;
            msg["role"] = message.role_;
            msg["content"] = message.content_;
            data_["messages"].append(msg);
        }
    }

    void OllamaConfig::clear_messages()
    {
        data_["messages"] = Json::Value(Json::arrayValue);
    }

    Json::Value OllamaConfig::get_messages() const
    {
        return get("messages");
    }

    // 返回整个Json
    const Json::Value &OllamaConfig::asJson() const
    {
        return data_;
    }
}
