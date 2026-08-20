#include "GeminiConfig.hpp"

namespace ai_chat_sdk
{
    // 构造函数
    GeminiConfig::GeminiConfig()
    {
        init_defaults();
    }
    void GeminiConfig::init_defaults()
    {
        // 默认模型
        data_["model"] = "gemini-3.5-flash";

        // 支持的参数
        data_["max_tokens"] = 4096;
        data_["temperature"] = 1.0;
        data_["stream"] = false;

        // 消息数组（空）
        data_["messages"] = Json::Value(Json::arrayValue);
    }
    // 设置模型完整URL
    void GeminiConfig::set_url(const std::string &endpoint, const std::string &path)
    {
        endpoint_ = endpoint;
        api_path_ = path;
    }
    // 获取服务器地址
    std::string GeminiConfig::get_endpoint()
    {
        return endpoint_;
    }
    // 获取模型具体路径
    std::string GeminiConfig::get_path()
    {
        return api_path_;
    }

    // 设置API_KEY
    void GeminiConfig::set_api_key(const std::string &api_key)
    {
        api_key_ = api_key;
    }
    // 获取API_KEY
    std::string GeminiConfig::get_api_key()
    {
        return api_key_;
    }

    // 通用设置接口
    bool GeminiConfig::set(const std::string &key, const Json::Value &value)
    {
        if (!data_.isMember(key))
        {
            LOG_WARN_STREAM() << "插入了非默认参数，请自行确定参数是否有效: " << key;
        }
        data_[key] = value;
        return true;
    }

    Json::Value GeminiConfig::get(const std::string &key) const
    {
        if (data_.isMember(key))
        {
            return data_[key];
        }
        return Json::Value::null;
    }

    // 便捷化接口
    void GeminiConfig::set_model(const std::string &model)
    {
        set("model", model);
    }
    std::string GeminiConfig::get_model() const
    {
        return get("model").asString();
    }

    void GeminiConfig::set_temperature(double temp)
    {
        set("temperature", temp);
    }
    double GeminiConfig::get_temperature() const
    {
        return get("temperature").asDouble();
    }

    void GeminiConfig::set_max_tokens(int tokens)
    {
        set("max_tokens", tokens);
    }
    int GeminiConfig::get_max_tokens() const
    {
        return get("max_tokens").asInt();
    }

    void GeminiConfig::set_stream(bool stream)
    {
        set("stream", stream);
    }
    bool GeminiConfig::get_stream() const
    {
        return get("stream").asBool();
    }

    void GeminiConfig::set_model_desc(const std::string &desc = "Gemini 为原生多模态智能体模型，具备深度推理能力、百万级长上下文窗口，并针对编程与复杂任务执行进行优化，致力于将AI从对话工具进化为可主动协助的工作流引擎")
    {
        desc_ = desc;
    }

    std::string GeminiConfig::get_model_desc() const
    {
        return desc_;
    }

    // 消息管理
    void GeminiConfig::set_messages(const std::vector<Message> &messages)
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

    void GeminiConfig::clear_messages()
    {
        data_["messages"] = Json::Value(Json::arrayValue);
    }

    Json::Value GeminiConfig::get_messages() const
    {
        return get("messages");
    }

    // 返回整个Json
    const Json::Value &GeminiConfig::asJson() const
    {
        return data_;
    }

    std::string GeminiConfig::get_series_name() const
    {
        return "gemini";
    }

    // 工厂函数，创建一个默认 config
    std::shared_ptr<GeminiConfig> get_gm_config()
    {
        auto gm_config = std::make_shared<GeminiConfig>();
        gm_config->set_url("https://generativelanguage.googleapis.com", "/v1beta/openai/chat/completions");
        gm_config->set_model_desc();
        return gm_config;
    }
}
