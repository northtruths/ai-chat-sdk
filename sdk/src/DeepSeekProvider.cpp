
#include "Common.hpp"
#include "DeepSeekProvider.hpp"
#include "utils/logger.hpp"
#include "DeepSeekConfig.hpp"
#include <string>
#include <jsoncpp/json/json.h>
#include <httplib.h>
#include <sstream>
#include <functional>


using namespace mylog;
namespace ai_chat_sdk
{
    DeepSeekProvider::DeepSeekProvider(std::shared_ptr<BaseConfig> cf)
    {
        set_model(cf);
    }

    // 设置模型配置
    bool DeepSeekProvider::set_model(std::shared_ptr<BaseConfig> cf)
    {
        config_ = cf;
        is_available_ = true;
        LOG_INFO("(DeepSeek) 模型配置设置成功");
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

    // 获取模型参数信息
    Json::Value DeepSeekProvider::get_params(const std::string &key) const
    {
        return config_->get(key);
    }

    // 全量式发送信息
    std::string DeepSeekProvider::send_message(const std::vector<Message> &messages)
    {
        // 检测模型是否有效
        if (is_available_ == false)
        {
            LOG_ERROR("(DeepSeek) 发送失败！模型无效！");
            return std::string();
        }

        // 构建json信息
        config_->set_messages(messages);
        config_->set_stream(false);
        Json::Value data = config_->asJson();

        // json序列化
        Json::FastWriter writer;
        std::string send_json_str = writer.write(data);
        LOG_DEBUG_STREAM() << "(deepseek) 发送信息json串为: " << send_json_str;

        // 通过http发送信息
        httplib::Client client(config_->get_endpoint());
        client.set_connection_timeout(30); // 给30s连接时间
        client.set_read_timeout(120);      // 给120s返回时间
        // 请求报头
        httplib::Headers headers = {{"Content-Type", "application/json"},
                                    {"Accept", "application/json"},
                                    {"Authorization", "Bearer " + config_->get_api_key()}};
        
        
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
                    LOG_ERROR("(DeepSeek) 信息发送失败！原因如下：URL未配置成功，请配置 endpoint 和 path");
                }
                else
                {
                    // 网络层错误（连接失败、超时等）
                    LOG_ERROR("(DeepSeek) 信息发送失败！原因如下：网络连接失败，请检查网络或代理设置");
                }
                return std::string();
            }else{
                log_error_code(res->status);
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
                    LOG_DEBUG_STREAM() << "返回信息为: " << resp_content;
                    return resp_content;
                }
                else
                {
                    LOG_DEBUG("没有 message 或 没有content");
                }
            }
            else
            {
                LOG_DEBUG("没有choices 或 choice 格式内容错误");
            }
            // 模型返回信息错误
            LOG_ERROR("(DeepSeek) 返回信息错误！");
            return std::string();
        }
    }

    // 流式发送信息
    std::string DeepSeekProvider::send_message_stream(const std::vector<Message> &messages, one_chunk callback)
    {
        // 检测模型是否有效
        if (is_available_ == false)
        {
            LOG_ERROR("(DeepSeek) 发送失败！模型无效！");
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
                                    {"Authorization", "Bearer " + config_->get_api_key()},
                                    {"Accept", "text/event-stream"}};

        // 获取返回信息并解析
        // buffer：拼接信息段的临时接收缓冲区
        std::string buffer;
        // full_content：完整的返回信息
        std::string full_content;
        // 状态码
        int status_code = 0;
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
                return false;
            }
            return true;
        };
        // 内容接收器：解析数据段，进行调用回调（输出内容）和拼接完整输出（构建历史信息）
        Json::CharReaderBuilder reader;//json反序列化，放外面避免频繁构建
        request.content_receiver = [&](const char *data, size_t len,
                                       uint64_t offset, uint64_t total)
        {
            if (status_code != 200)
            {
                LOG_WARN("(DeepSeek) 程序出错！响应处理器未正确运行或内容接收器运行出错！");
                log_error_code(status_code);
                return false;
            }
            // 内容进入缓冲区
            buffer.append(data, len);
            // 判断、解析当前完整一段 数据体
            int pos;

            while ((pos = buffer.find("\n\n")) != std::string::npos)
            {
                // 切割一段完整数据
                std::string event = buffer.substr(0, pos);
                LOG_DEBUG_STREAM() << "待解析信息段: " << event;
                buffer.erase(0, pos + 2);
                // 处理空行和注释行
                if (event.empty() || event[0] == ':')
                {
                    LOG_DEBUG("无效段，跳过啦");
                    continue;
                }
                // 检查事件类型
                if (event.substr(0, 6) == "data: ")
                {
                    std::string resp_json_str = event.substr(6);
                    //LOG_DEBUG_STREAM() << "事件原始json字符串：" << resp_json_str;
                    // 解析数据
                    if (resp_json_str == "[DONE]")
                    {
                        callback("", true);
                        LOG_INFO("信息发送完毕");
                        return false;
                    }
                    Json::Value chunk_json;
                    std::string prase_error;
                    std::istringstream stream(resp_json_str);
                    if (!Json::parseFromStream(reader, stream, &chunk_json, &prase_error))
                    {
                        // 返回信息解析失败
                        LOG_ERROR_STREAM() << "(DeepSeek) 返回信息解析失败：" << prase_error;
                        return false;
                    }
                    // 严格检查
                    if (chunk_json.isMember("choices") && chunk_json["choices"].isArray() && !chunk_json["choices"].empty())
                    {
                        auto &choice = chunk_json["choices"][0];
                        if (choice.isMember("delta") && choice["delta"].isMember("content"))
                        {
                            std::string resp_chunk = choice["delta"]["content"].asString();
                            LOG_INFO_STREAM() << "(DeepSeek) 信息段解析成功: " << resp_chunk;
                            callback(resp_chunk, false);
                            // 拼接完整信息
                            full_content += resp_chunk;
                        }
                        else
                        {
                            LOG_DEBUG("(DeepSeek) json格式不符合预期");
                        }
                    }
                    else
                    {
                        LOG_DEBUG("(DeepSeek) json格式不符合预期");
                    }
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
                LOG_ERROR("(DeepSeek) 信息发送失败！原因如下：URL未配置成功，请配置 endpoint 和 path");
            }
            else
            {
                // 网络层错误（连接失败、超时等）
                LOG_ERROR("(DeepSeek) 信息发送失败！原因如下：网络连接失败，请检查网络或代理设置");
            }
            return std::string();
        }
        LOG_INFO("完整信息发送完毕");
        return full_content;
    }
}