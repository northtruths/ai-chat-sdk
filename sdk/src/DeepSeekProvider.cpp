
#include "Common.hpp"
#include "DeepSeekProvider.hpp"
#include "utils/logger.hpp"
#include "DeepSeekConfig.hpp"
#include <string>
#include <jsoncpp/json/json.h>
#include <httplib.h>
#include <sstream>

using namespace mylog;
namespace ai_chat_sdk
{
    DeepSeekProvider::DeepSeekProvider(BaseConfig *cf)
    {
        init_model(cf);
    }

    // 初始化模型
    bool DeepSeekProvider::init_model(BaseConfig *cf)
    {
        if (is_available_)
        {
            LOG_WARN("模型已经初始化");
            return false;
        }
        config_ = cf;
        is_available_ = true;
        LOG_INFO("(DeepSeek) 模型初始化成功");
        return true;
    }

    // 检测模型是否有效
    bool DeepSeekProvider::is_available() const
    {
        return is_available_;
    }

    // 获取模型名称
    std::string DeepSeekProvider::get_model() const
    {
        return config_->get_model();
    }
    // 获取模型描述
    std::string DeepSeekProvider::get_model_desc() const
    {
        return config_->get_model_desc();
    }

    // 更改配置参数
    bool DeepSeekProvider::set_params(const std::string &key, const Json::Value &value)
    {
        return config_->set(key, value);
    }
    // 获取模型配置信息
    Json::Value DeepSeekProvider::get_params(const std::string &key) const
    {
        return config_->get(key);
    }

    // 全量式发送信息
    std::string DeepSeekProvider::send_message(const std::string content)
    {
        // 检测模型是否有效
        if (is_available_ == false)
        {
            LOG_ERROR("(DeepSeek) 发送失败！模型无效！");
            return std::string();
        }

        // 构建json信息
        config_->add_message("user", content);
        Json::Value data = config_->asJson();

        // json序列化
        Json::FastWriter writer;
        std::string json_str = writer.write(data);

        // 通过http发送信息
        httplib::Client client(config_->get_endpoint());
        client.set_connection_timeout(30); // 给30s连接时间
        client.set_read_timeout(120);      // 给120s返回时间
        // 请求报头
        httplib::Headers headers = {{"Content-Type", "application/json"},
                                    {"Accept", "application/json"},
                                    {"Authorization", "Bearer " + config_->get_api_key()}};
        auto res = client.Post(config_->get_path(), headers, json_str, "application/json");

        // 获取返回信息并解析
        // 没有成功响应，解析对应错误
        if (!res || res->status != 200)
        {
            if (!res)
            {
                if (config_->get_endpoint().empty() || config_->get_path().empty())
                {
                    // 没有配置好url
                    LOG_ERROR("(DeepSeek) 信息发送失败！原因如下：URL未配置成功，请配置 endpoint 和 path");
                }
                else
                {
                    // 网络层错误（连接失败、超时等）
                    LOG_ERROR("(DeepSeek) 信息发送失败！原因如下：网络连接失败，请检查网络或代理设置");
                }
                return std::string();
            }
            switch (res->status)
            {
            case 401:
                LOG_ERROR("(DeepSeek) 信息发送失败！原因如下：检查 API Key 是否正确或已过期");
                break;
            case 403:
                LOG_ERROR("(DeepSeek) 信息发送失败！原因如下：账户权限不足，或 API Key 无权限访问该模型");
                break;
            case 404:
                LOG_ERROR("(DeepSeek) 信息发送失败！原因如下：检查请求路径是否正确，或模型名称是否存在");
                break;
            case 429:
                LOG_ERROR("(DeepSeek) 信息发送失败！原因如下：请求过于频繁，请稍后重试");
                break;
            case 500:
            case 502:
            case 503:
                LOG_ERROR("(DeepSeek) 信息发送失败！原因如下：DeepSeek 服务器内部错误，请稍后重试");
                break;
            default:
                break;
            }
            return std::string();
        }
        LOG_DEBUG_STREAM() << "DeepSeek 响应体: " << res->body;
        // 解析返回消息
        Json::Value resp_json;
        std::string prase_error;
        Json::CharReaderBuilder reader;
        std::istringstream stream(res->body);
        if (!Json::parseFromStream(reader, stream, &resp_json, &prase_error))
        {
            // 返回信息解析失败
            LOG_ERROR_STREAM() << "(DeepSeek) 返回信息解析失败：" << prase_error;
            return std::string();
        }
        // 返回消息解析成功，解析大模型返回内容
        {
            // 严格检查
            if (resp_json.isMember("choices") && resp_json["choices"].isArray() && !resp_json["choices"].empty())
            {
                auto &choice = resp_json["choices"][0];
                if (choice.isMember("message") && choice["message"].isMember("content"))
                {
                    std::string resp_content = choice["message"]["content"].asString();
                    LOG_INFO("(DeepSeek) 信息返回成功");
                    LOG_DEBUG_STREAM() << "返回信息为：" << resp_content;
                    return resp_content;
                }else{
                    LOG_DEBUG("没有 message 或 没有content");
                }
            }else{
                LOG_DEBUG("没有choices 或 choice 格式内容错误");
            }
            // 模型返回信息错误
            LOG_ERROR("(DeepSeek) 返回信息错误！");
            return std::string();
        }
    }

    // 流式发送信息
    std::string DeepSeekProvider::send_meesage_stream(const std::string content)
    {
        std::string temp = content;
        return std::string();
    }
}