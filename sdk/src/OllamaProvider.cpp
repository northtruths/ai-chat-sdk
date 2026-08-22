
#include "Common.hpp"
#include "OllamaProvider.hpp"
#include "utils/logger.hpp"
#include "OllamaConfig.hpp"
#include <string>
#include <jsoncpp/json/json.h>
#include <httplib.h>
#include <sstream>
#include <functional>

using namespace mylog;
namespace ai_chat_sdk
{
    OllamaProvider::OllamaProvider(std::shared_ptr<BaseConfig> cf)
    {
        set_model(cf);
    }

    // 设置模型配置
    bool OllamaProvider::set_model(std::shared_ptr<BaseConfig> cf)
    {
        if (is_available_)
        {
            LOG_WARN("模型已经初始化");
            return false;
        }
        config_ = cf;
        is_available_ = true;
        LOG_INFO("(Ollama) 模型初始化成功");
        return true;
    }

    // 检测模型是否有效
    bool OllamaProvider::is_available() const
    {
        return is_available_;
    }

    // 获取模型名称
    std::string OllamaProvider::get_model() const
    {
        return config_->get_model();
    }
    // 获取模型描述
    std::string OllamaProvider::get_model_desc() const
    {
        return config_->get_model_desc();
    }

    // 获取模型配置信息
    Json::Value OllamaProvider::get_params(const std::string &key) const
    {
        return config_->get(key);
    }

    // 全量式发送信息
    std::string OllamaProvider::send_message(const std::vector<Message> &messages)
    {
        // 检测模型是否有效
        if (is_available_ == false)
        {
            LOG_ERROR("(Ollama) 发送失败！模型无效！");
            return std::string();
        }

        // 构建json信息
        config_->set_messages(messages);
        config_->set_stream(false);
        Json::Value data = config_->asJson();

        // json序列化
        Json::FastWriter writer;
        std::string send_json_str = writer.write(data);

        // 通过http发送信息
        httplib::Client client(config_->get_endpoint());
        client.set_connection_timeout(30); // 给30s连接时间
        client.set_read_timeout(120);      // 给120s返回时间
        // 请求报头
        httplib::Headers headers = {{"Content-Type", "application/json"},
                                    {"Accept", "application/json"}};
        auto res = client.Post(config_->get_path(), headers, send_json_str, "application/json");

        // 获取返回信息并解析
        // 没有成功响应，解析对应错误
        if (!res || res->status != 200)
        {
            if (!res)
            {
                if (config_->get_endpoint().empty() || config_->get_path().empty())
                {
                    // 没有配置好url
                    LOG_ERROR("(Ollama) 信息发送失败！原因如下：URL未配置成功，请配置 endpoint 和 path");
                }
                else
                {
                    // 网络层错误（连接失败、超时等）
                    LOG_ERROR_STREAM() << "(Ollama) 信息发送失败！HTTP错误: "
                                       << httplib::to_string(res.error());
                }
                return std::string();
            }
            else
            {
                LOG_DEBUG_STREAM() << "Ollama 响应体: " << res->body;
                log_error_code(res->status);
            }
            return std::string();
        }
        LOG_DEBUG_STREAM() << "Ollama 响应体: " << res->body;
        // 解析返回消息
        Json::Value resp_json;
        std::string prase_error;
        Json::CharReaderBuilder reader;
        std::istringstream stream(res->body);
        if (!Json::parseFromStream(reader, stream, &resp_json, &prase_error))
        {
            // 返回信息解析失败
            LOG_ERROR_STREAM() << "(Ollama) 返回信息解析失败：" << prase_error;
            return std::string();
        }
        // 返回消息解析成功，解析大模型返回内容
        {
            // 如果返回json中有字段error则说明模型使用错误
            if (resp_json.isMember("error"))
            {
                LOG_ERROR_STREAM() << "error: " << resp_json["error"].asString();
            }
            else
            {
                if (resp_json.isMember("message") && resp_json["message"].isMember("content"))
                {
                    std::string resp_content = resp_json["message"]["content"].asString();
                    LOG_INFO("(Ollama) 信息返回成功");
                    return resp_content;
                }
                else
                {
                    LOG_DEBUG("没有 message 或 没有content");
                }
            }

            return std::string();
        }
    }

    // 流式发送信息
    std::string OllamaProvider::send_message_stream(const std::vector<Message> &messages, one_chunk callback)
    {
        // 检测模型是否有效
        if (is_available_ == false)
        {
            LOG_ERROR("(Ollama) 发送失败！模型无效！");
            return std::string();
        }

        // 构建json信息
        config_->set_messages(messages);
        config_->set_stream(true);
        Json::Value data = config_->asJson();

        // json序列化
        Json::FastWriter writer;
        std::string send_json_str = writer.write(data);
        LOG_DEBUG_STREAM() << "send_json_str: " << send_json_str;

        // 通过http发送信息
        httplib::Client client(config_->get_endpoint());
        client.set_connection_timeout(30); // 给30s连接时间
        client.set_read_timeout(120);      // 给120s返回时间
        // 请求报头
        httplib::Headers headers = {{"Content-Type", "application/json"},
                                    {"Accept", "text/event-stream"}};

        // 获取返回信息并解析
        // buffer：拼接信息段的临时接收缓冲区
        std::string buffer;
        // full_content：完整的返回信息
        std::string full_content;
        // 状态码
        int status_code = 0;
        // 错误信息
        std::string error_body;
        bool is_error = false;
        // 构建 Request 对象，设置 response_handler 和 content_receiver
        httplib::Request request;
        request.method = "POST";
        request.path = config_->get_path();
        request.headers = headers;
        request.body = send_json_str;
        // 响应处理器，进行判断 http 响应状态
        request.response_handler = [&](const httplib::Response &response)
        {
            status_code = response.status;
            if (status_code != 200)
            {
                log_error_code(status_code);
                is_error = true;
                return true;
            }
            return true;
        };
        // 内容接收器：解析数据段，进行调用回调（输出内容）和拼接完整输出（构建历史信息）
        Json::CharReaderBuilder reader; // json反序列化，放外面避免频繁构建
        request.content_receiver = [&](const char *data, size_t len,
                                       uint64_t offset, uint64_t total)
        {
            if (is_error)
            {
                error_body.append(data, len);
                LOG_DEBUG_STREAM() << "Gemini 响应体: " << error_body;
                return true;
            }
            // 内容进入缓冲区
            buffer.append(data, len);
            LOG_DEBUG_STREAM() << "data: { " << buffer << " }";

            // 判断、解析当前完整一段 数据体
            int pos;

            while ((pos = buffer.find("\n")) != std::string::npos)
            {
                // 切割一段完整数据
                std::string event = buffer.substr(0, pos);
                LOG_DEBUG_STREAM() << "待解析信息段: " << event;
                buffer.erase(0, pos + 1);
                // 处理空行和注释行
                if (event.empty() || event[0] == ':')
                {
                    LOG_DEBUG("无效段，跳过啦");
                    continue;
                }
                // event就是原始返回json_str了，没有像deepseek-v4-pro那样的 data: 额外格式需要分割一下
                //   解析数据
                Json::Value chunk_json;
                std::string prase_error;
                std::istringstream stream(event);
                if (!Json::parseFromStream(reader, stream, &chunk_json, &prase_error))
                {
                    // 返回信息解析失败
                    LOG_ERROR_STREAM() << "(Ollama) 返回信息解析失败：" << prase_error;
                    return false;
                }

                // 严格检查
                if (chunk_json.isMember("done"))
                {
                    if (chunk_json["done"] == true)
                    {
                        callback("", true);
                        LOG_INFO("信息发送完毕");
                        return false;
                    }
                    if (chunk_json.isMember("message") && chunk_json["message"].isMember("content"))
                    {
                        std::string resp_chunk = chunk_json["message"]["content"].asString();
                        LOG_INFO_STREAM() << "(Ollama) 信息段解析成功: " << resp_chunk;
                        callback(resp_chunk, false);
                        // 拼接完整信息
                        full_content += resp_chunk;
                    }
                    else
                    {
                        LOG_DEBUG("(Ollama) json格式不符合预期, 没有具体消息");
                    }
                }
                else
                {
                    LOG_DEBUG("(Ollama) json格式不符合预期, 没有具体消息");
                }
            }

            return true;
        };

        // 发送信息
        auto res = client.send(request);

        // 信息在网络层断掉，没有成功发送出去
        if (!res && status_code == 0)
        {
            if (config_->get_endpoint().empty() || config_->get_path().empty())
            {
                // 没有配置好url
                LOG_ERROR("(Ollama) 信息发送失败！原因如下：URL未配置成功，请配置 endpoint 和 path");
            }
            else
            {
                // 网络层错误（连接失败、超时等）
                LOG_ERROR_STREAM() << "(Ollama) 信息发送失败！HTTP错误: "
                                   << httplib::to_string(res.error());
            }
            return std::string();
        }
        LOG_INFO("完整信息发送完毕");
        return full_content;
    }
}