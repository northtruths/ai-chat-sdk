#include "ChatGPTConfig.hpp"

namespace ai_chat_sdk
{
    // 构造函数
    ChatGPTConfig::ChatGPTConfig()
    {
        init_defaults();
    }
    void ChatGPTConfig::init_defaults()
    {
        // 默认模型
        data_["model"] = "gpt-5.4";

        // 支持的参数
        data_["max_tokens"] = 4096;
        data_["temperature"] = 1.0;
        data_["stream"] = false;

        // 消息数组（空）
        data_["messages"] = Json::Value(Json::arrayValue);
    }
    // 设置模型完整URL
    void ChatGPTConfig::set_url(const std::string &endpoint, const std::string &path)
    {
        endpoint_ = endpoint;
        api_path_ = path;
    }
    // 获取服务器地址
    std::string ChatGPTConfig::get_endpoint()
    {
        return endpoint_;
    }
    // 获取模型具体路径
    std::string ChatGPTConfig::get_path()
    {
        return api_path_;
    }

    // 设置API_KEY
    void ChatGPTConfig::set_api_key(const std::string &api_key)
    {
        api_key_ = api_key;
    }
    // 获取API_KEY
    std::string ChatGPTConfig::get_api_key()
    {
        return api_key_;
    }

    // 通用设置接口
    bool ChatGPTConfig::set(const std::string &key, const Json::Value &value)
    {
        if (!data_.isMember(key))
        {
            LOG_WARN_STREAM() << "插入了非默认参数，请自行确定参数是否有效: " << key;
        }
        data_[key] = value;
        return true;
    }

    Json::Value ChatGPTConfig::get(const std::string &key) const
    {
        if (data_.isMember(key))
        {
            return data_[key];
        }
        return Json::Value::null;
    }

    // 便捷化接口
    void ChatGPTConfig::set_model(const std::string &model)
    {
        set("model", model);
    }
    std::string ChatGPTConfig::get_model() const
    {
        return get("model").asString();
    }

    void ChatGPTConfig::set_temperature(double temp)
    {
        set("temperature", temp);
    }
    double ChatGPTConfig::get_temperature() const
    {
        return get("temperature").asDouble();
    }

    void ChatGPTConfig::set_max_tokens(int tokens)
    {
        set("max_tokens", tokens);
    }
    int ChatGPTConfig::get_max_tokens() const
    {
        return get("max_tokens").asInt();
    }

    void ChatGPTConfig::set_stream(bool stream)
    {
        set("stream", stream);
    }
    bool ChatGPTConfig::get_stream() const
    {
        return get("stream").asBool();
    }

    void ChatGPTConfig::set_model_desc(const std::string &desc = "ChatGPT 为通用型对话式人工智能模型，具备强大的自然语言理解与生成能力、超长文本处理能力，并支持丰富的插件生态与多模态扩展。它致力于将 AI 从单一问答工具进化为全场景的智能协作平台。")
    {
        desc_ = desc;
    }

    std::string ChatGPTConfig::get_model_desc() const
    {
        return desc_;
    }

    // 消息管理
    void ChatGPTConfig::set_messages(const std::vector<Message> &messages)
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

    void ChatGPTConfig::clear_messages()
    {
        data_["messages"] = Json::Value(Json::arrayValue);
    }

    Json::Value ChatGPTConfig::get_messages() const
    {
        return get("messages");
    }

    // 返回整个Json
    const Json::Value &ChatGPTConfig::asJson() const
    {
        return data_;
    }

    std::string ChatGPTConfig::get_series_name() const
    {
        return "gemini";
    }

    // 工厂函数，创建一个默认 config
    std::shared_ptr<ChatGPTConfig> get_gpt_config()
    {
        auto gm_config = std::make_shared<ChatGPTConfig>();
        gm_config->set_url("https://api.openai.com", "/v1/chat/completions");
        return gm_config;
    }
}
