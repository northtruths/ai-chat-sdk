#include "LLMManager.hpp"
#include "utils/logger.hpp"

namespace ai_chat_sdk
{
    // 注册 Provider
    bool LLMManager::register_provider(const std::string &name, std::unique_ptr<LLMProvider> provider)
    {
        if (!provider)
        {
            LOG_WARN("注册模型无效");
            return false;
        }
        providers_[name] = std::move(provider);
        return true;
    }

    // 获取 Provider
    LLMProvider *LLMManager::get_provider(const std::string &name) const
    {
        auto it = providers_.find(name);
        if (it == providers_.end())
        {
            LOG_WARN_STREAM() << "Provider {" << name << "} 未注册";
            return nullptr;
        }
        return it->second.get();
    }

    // 发送消息（非流式）
    std::string LLMManager::send_message(const std::string &modelName,
                                         const std::vector<Message> &messages)
    {
        if (messages.empty())
        {
            LOG_WARN("发送信息为空，发送失败");
            return std::string();
        }
        LLMProvider *pd = get_provider(modelName);
        if (!pd)
        {
            LOG_WARN("发送失败，未找到正确模型");
            return std::string();
        }
        LOG_DEBUG_STREAM() << "发送信息为: " << messages.back().content_;
        return pd->send_message(messages);
    }

    // 发送消息（流式）
    std::string LLMManager::send_message_stream(const std::string &modelName,
                                                const std::vector<Message> &messages,
                                                std::function<void(const std::string &, bool)> callback)
    {
        if (messages.empty())
        {
            LOG_WARN("发送信息为空，发送失败");
            return std::string();
        }
        LLMProvider *pd = get_provider(modelName);
        if (!pd)
        {
            LOG_WARN("发送失败，未找到正确模型");
            return std::string();
        }
        LOG_DEBUG_STREAM() << "发送信息为: " << messages.back().content_;
        return pd->send_message_stream(messages, callback);
    }

    // 获取可用模型信息
    std::vector<ModelInfo> LLMManager::get_available_models() const
    {

        std::vector<ModelInfo> infos;
        for (auto &pd : providers_)
        {
            ModelInfo temp;
            temp.is_available_ = true;
            temp.model_name_ = pd.second->get_model();
            temp.model_desc_ = pd.second->get_model_desc();
            temp.provider_ = pd.first;
            infos.push_back(temp);
        }
        return infos;
    }

    // 检查模型是否可用
    bool LLMManager::is_model_available(const std::string &name) const
    {
        auto provider = get_provider(name);
        return provider->is_available();
    }

    // 获取所有已注册模型模型
    std::vector<std::string> LLMManager::get_registed_models() const
    {
        std::vector<std::string> registers;
        for (auto &it : providers_)
        {
            registers.push_back(it.first);
        }
        return registers;
    }
}