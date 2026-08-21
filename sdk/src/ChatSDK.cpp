// ChatSdk.cpp

#include "ChatSDK.hpp"
#include "DeepSeekConfig.hpp"
#include "DeepSeekProvider.hpp"
#include "OllamaConfig.hpp"
#include "OllamaProvider.hpp"
#include "GeminiConfig.hpp"
#include "GeminiProvider.hpp"
#include "ChatGPTConfig.hpp"
#include "ChatGPTProvider.hpp"

namespace ai_chat_sdk
{
    // 构造函数
    ChatSDK::ChatSDK()
        : llm_manager_(std::make_unique<LLMManager>()), session_manager_(std::make_unique<SessionManager>())
    {
    }
    ChatSDK::ChatSDK(std::string db_path = "./chat.db")
        : llm_manager_(std::make_unique<LLMManager>()), session_manager_(std::make_unique<SessionManager>(db_path))
    {
    }

    // 初始化模型
    bool ChatSDK::init_models(std::vector<std::shared_ptr<BaseConfig>> configs)
    {
        for (auto &config : configs)
        {
            configs_[config->get_series_name()] = config;
        }
        register_all_provider();
        initialized_ = true;

        LOG_DEBUG("(ChatSdk) 初始化模型成功");
        return true;
    }

    // 获取默认 configs
    std::vector<std::shared_ptr<BaseConfig>> ChatSDK::get_default_configs()
    {
        std::vector<std::shared_ptr<BaseConfig>> configs;

        configs.push_back(get_ds_config());  // 第1个：DeepSeek
        configs.push_back(get_gpt_config()); // 第2个：ChatGPT
        configs.push_back(get_gm_config());  // 第3个：Gemini
        configs.push_back(get_ol_config());  // 第4个：Ollama

        return configs;
    }

    // 创建session
    std::string ChatSDK::create_session(const std::string &model_name)
    {
        return session_manager_->create_session(model_name);
    }

    // 获取会话
    std::shared_ptr<Session> ChatSDK::get_session(const std::string &session_id)
    {
        return session_manager_->get_session(session_id);
    }

    // 按时间降序获取所有会话列表 (会话 id)
    std::vector<std::string> ChatSDK::get_session_list()
    {
        return session_manager_->get_session_list();
    }

    // 删除会话
    bool ChatSDK::delete_session(const std::string &session_id)
    {
        return session_manager_->delete_session(session_id);
    }

    // 获取可⽤模型列表
    std::vector<ModelInfo> ChatSDK::get_available_models()
    {
        return llm_manager_->get_available_models();
    }

    // 发送消息 - 全量返回
    std::string ChatSDK::send_message(const std::string session_id, const std::string &message)
    {
        bool ok;
        std::string reply;
        auto session = session_manager_->get_session(session_id);
        if (!session)
        {
            LOG_WARN("(ChatSdk) 会话不存在，消息发送失败");
            return reply;
        }

        ok = session_manager_->add_message(session_id, "user", message);
        if (!ok)
        {
            LOG_WARN("(ChatSdk) 发送消息存储失败");
            LOG_WARN("(ChatSdk) 消息发送失败");
            return reply;
        }

        reply = llm_manager_->send_message(session->model_name_, session->messages_);
        if (reply.empty())
        {
            LOG_WARN("(ChatSdk) 消息发送/返回失败");
            return reply;
        }

        ok = session_manager_->add_message(session_id, "assistant", reply);
        if (!ok)
        {
            LOG_WARN("(ChatSdk) 返回消息存储失败");
        }

        return reply;
    }

    // 发送消息 - 流式响应 (callback 为每个返回的数据块的处理方式，和 是否是末尾数据的结束标志)
    std::string ChatSDK::send_message_stream(const std::string session_id, const std::string &message, std::function<void(const std::string &, bool)> callback)
    {
        bool ok;
        std::string reply;
        auto session = session_manager_->get_session(session_id);
        if (!session)
        {
            LOG_WARN("(ChatSdk) 会话不存在，消息发送失败");
            return reply;
        }

        ok = session_manager_->add_message(session_id, "user", message);
        if (!ok)
        {
            LOG_WARN("(ChatSdk) 发送消息存储失败");
            LOG_WARN("(ChatSdk) 消息发送失败");
            return reply;
        }

        reply = llm_manager_->send_message_stream(session->model_name_, session->messages_, callback);
        if (reply.empty())
        {
            LOG_WARN("(ChatSdk) 消息发送/返回失败");
            return reply;
        }

        ok = session_manager_->add_message(session_id, "assistant", reply);
        if (!ok)
        {
            LOG_WARN("(ChatSdk) 返回消息存储失败");
        }

        return reply;
    }

    // 注册所有模型
    void ChatSDK::register_all_provider()
    {
        for (auto &[model_name, config] : configs_)
        {
            if (model_name == "deepseek")
            {
                auto provider = std::make_unique<DeepSeekProvider>(config);
                llm_manager_->register_provider("deepseek", std::move(provider));
            }
            else if (model_name == "ollama")
            {
                auto provider = std::make_unique<OllamaProvider>(config);
                llm_manager_->register_provider("ollama", std::move(provider));
            }
            else if (model_name == "chatgpt")
            {
                auto provider = std::make_unique<ChatGPTProvider>(config);
                llm_manager_->register_provider("chatgpt", std::move(provider));
            }
            else if (model_name == "gemini")
            {
                auto provider = std::make_unique<GeminiProvider>(config);
                llm_manager_->register_provider("gemini", std::move(provider));
            }
            else
            {
                LOG_WARN("注册模型中存在未知/不支持模型");
            }
        }
    }

} // ai_chat_sdk