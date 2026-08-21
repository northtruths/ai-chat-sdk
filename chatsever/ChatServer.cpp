#include "ChatServer.hpp"
#include <ai_chat_sdk/utils/logger.hpp>
#include <ai_chat_sdk/APIConfig.hpp>
#include <httplib.h>
#include <jsoncpp/json/forwards.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/writer.h>

namespace ai_chat_server
{

    ChatServer::ChatServer(const ServerConfig &config)
        : config_(config)
    {
        chat_sdk_ = std::make_shared<ai_chat_sdk::ChatSDK>();

        // deepseek-v4-pro
        auto deepseek_config = ai_chat_sdk::get_ds_config();
        deepseek_config->set_model("deepseek-v4-pro");
        deepseek_config->set_api_key(config_.deepseek_api_key_);
        deepseek_config->set_temperature(config_.temperature_);
        deepseek_config->set_max_tokens(config_.max_tokens_);

        // gpt-5.4-mini
        auto chatgpt_config = ai_chat_sdk::get_gpt_config();
        chatgpt_config->set_model("gpt-5.4-mini");
        chatgpt_config->set_api_key(config_.chatgpt_api_key_);
        chatgpt_config->set_temperature(config_.temperature_);
        chatgpt_config->set_max_tokens(config_.max_tokens_);

        // gemini-3.5-flash
        auto gemini_config = ai_chat_sdk::get_gm_config();
        gemini_config->set_model("gemini-3.5-flash");
        gemini_config->set_api_key(config_.gemini_api_key_);
        gemini_config->set_temperature(config_.temperature_);
        gemini_config->set_max_tokens(config_.max_tokens_);

        // Ollama本地接入
        auto ollama_config = std::make_shared<ai_chat_sdk::OllamaConfig>();
        ollama_config->set_model(config_.ollama_model_name_);
        ollama_config->set_model_desc(config_.ollama_model_desc_);
        ollama_config->set_url(config_.ollama_endpoint_, config_.ollama_path_);
        ollama_config->set_temperature(config_.temperature_);
        ollama_config->set_max_tokens(config_.max_tokens_);

        std::vector<std::shared_ptr<ai_chat_sdk::BaseConfig>> model_configs{
            std::static_pointer_cast<ai_chat_sdk::BaseConfig>(deepseek_config),
            std::static_pointer_cast<ai_chat_sdk::BaseConfig>(chatgpt_config),
            std::static_pointer_cast<ai_chat_sdk::BaseConfig>(gemini_config),
            std::static_pointer_cast<ai_chat_sdk::BaseConfig>(ollama_config)
        };

        LOG_INFO("start init ChatSDK models...");
        if (!chat_sdk_->init_models(model_configs))
        {
            LOG_ERROR("ChatSDK init Failed!!!");
            return;
        }
        LOG_INFO("ChatSDK models init success!!!");

        // 创建http服务器
        http_server_ = std::make_unique<httplib::Server>();
        if (!http_server_)
        {
            LOG_ERROR("ChatServer init Failed!!!");
            return;
        }
    }

    bool ChatServer::start()
    {
        if (is_running_.load())
        {
            LOG_ERROR("ChatServer is running!!!");
            return false;
        }

        // 设置路由规则
        set_http_routes();

        // 设置静态资源的路径
        http_server_->set_mount_point("/", "./www");

        // 服务器在单独的线程中运行
        std::thread server_thread([this]()
                                  {
            http_server_->listen(config_.host_, config_.port_);
            LOG_INFO_STREAM() << "ChatServer start on " << config_.host_ << ":" << config_.port_; });

        server_thread.detach();
        is_running_.store(true);
        LOG_INFO("ChatServer start success!!!");
        return true;
    }

    void ChatServer::stop()
    {
        if (!is_running_.load())
        {
            LOG_ERROR("ChatServer is not running!!!");
            return;
        }

        if (http_server_)
        {
            http_server_->stop();
        }

        is_running_.store(false);
        LOG_INFO("ChatServer stop success!!!");
    }

    bool ChatServer::is_running() const
    {
        return is_running_.load();
    }

    // 构造响应
    std::string ChatServer::build_response(const std::string &message, bool success)
    {
        Json::Value response_json;
        response_json["success"] = success;
        response_json["message"] = message;

        Json::StreamWriterBuilder writer_builder;
        return Json::writeString(writer_builder, response_json);
    }

    // 处理创建会话请求
    void ChatServer::handle_create_session_request(const httplib::Request &request, httplib::Response &response)
    {
        // 获取请求参数，请求参数在请求体
        // 通过反序列化拿到请求体的json格式
        Json::Value request_json;
        Json::Reader reader;
        if (!reader.parse(request.body, request_json))
        {
            std::string error_json_str = build_response("parse request body failed, json format error");
            response.status = 400; // 客户端发送的请求有语法错误，服务器无法理解或处理该请求
            response.set_content(error_json_str, "application/json");
            return;
        }

        // 获取请求参数
        std::string model_name = request_json.get("model", "deepseek-chat").asString();

        // 创建会话
        std::string session_id = chat_sdk_->create_session(model_name);
        if (session_id.empty())
        {
            std::string error_json_str = build_response("create session failed");
            response.status = 500; // 服务器内部错误，无法完成请求
            response.set_content(error_json_str, "application/json");
            return;
        }

        // 构建响应体
        Json::Value data_json;
        data_json["session_id"] = session_id;
        data_json["model"] = model_name;

        Json::Value response_json;
        response_json["success"] = true;
        response_json["message"] = "create session success";
        response_json["data"] = data_json;

        // 序列化
        Json::StreamWriterBuilder writer_builder;
        std::string response_json_str = Json::writeString(writer_builder, response_json);

        response.status = 200; // 成功
        response.set_content(response_json_str, "application/json");
    }

    // 处理获取会话列表请求
    void ChatServer::handle_get_session_lists_request(const httplib::Request &request, httplib::Response &response)
    {
        // 获取会话列表
        std::vector<std::string> session_ids = chat_sdk_->get_session_list();

        // 构建session信息
        Json::Value data_array(Json::arrayValue);
        for (const auto &session_id : session_ids)
        {
            auto session = chat_sdk_->get_session(session_id);
            if (session)
            {
                Json::Value session_json;
                session_json["id"] = session->id_;
                session_json["model"] = session->model_name_;
                session_json["created_at"] = static_cast<int64_t>(session->created_at_);
                session_json["updated_at"] = static_cast<int64_t>(session->updated_at_);
                session_json["message_count"] = static_cast<int>(session->messages_.size());
                if (!session->messages_.empty())
                {
                    session_json["first_user_message"] = session->messages_.front().content_;
                }

                data_array.append(session_json);
            }
        }

        // 构建响应体
        Json::Value response_json;
        response_json["success"] = true;
        response_json["message"] = "get session lists success";
        response_json["data"] = data_array;

        // 序列化
        Json::StreamWriterBuilder writer_builder;
        std::string response_json_str = Json::writeString(writer_builder, response_json);

        response.status = 200; // 成功
        response.set_content(response_json_str, "application/json");
    }

    // 处理获取模型列表请求
    void ChatServer::handle_get_model_lists_request(const httplib::Request &request, httplib::Response &response)
    {
        // 获取支持的模型列表
        auto model_lists = chat_sdk_->get_available_models();

        // 构建响应体
        Json::Value data_array(Json::arrayValue);
        for (const auto &model_info : model_lists)
        {
            Json::Value model_json;
            model_json["name"] = model_info.model_name_;
            model_json["provider"] = model_info.provider_;
            model_json["desc"] = model_info.model_desc_;
            data_array.append(model_json);
        }

        // 构建响应体
        Json::Value response_json;
        response_json["success"] = true;
        response_json["message"] = "get model lists success";
        response_json["data"] = data_array;

        // 序列化
        Json::StreamWriterBuilder writer_builder;
        std::string response_json_str = Json::writeString(writer_builder, response_json);

        response.status = 200; // 成功
        response.set_content(response_json_str, "application/json");
    }

    // 处理删除会话请求
    void ChatServer::handle_delete_session_request(const httplib::Request &request, httplib::Response &response)
    {
        // 获取会话id，注意：会话id是一个路径参数
        std::string session_id = request.matches[1];

        // 删除会话
        bool ret = chat_sdk_->delete_session(session_id);
        if (ret)
        {
            std::string success_json_str = build_response("delete session success", true);
            response.status = 200;
            response.set_content(success_json_str, "application/json");
        }
        else
        {
            std::string error_json_str = build_response("delete session failed, session not found");
            response.status = 404; // 会话不存在
            response.set_content(error_json_str, "application/json");
        }
    }

    // 处理获取历史消息请求
    void ChatServer::handle_get_history_messages_request(const httplib::Request &request, httplib::Response &response)
    {
        // 获取会话id
        std::string session_id = request.matches[1];
        // 获取会话
        auto session = chat_sdk_->get_session(session_id);
        if (!session)
        {
            std::string error_json_str = build_response("session not found");
            response.status = 404; // 会话不存在
            response.set_content(error_json_str, "application/json");
            return;
        }

        // 构建历史消息列表
        Json::Value data_array(Json::arrayValue);
        for (const auto &message : session->messages_)
        {
            Json::Value message_json;
            message_json["id"] = message.id_;
            message_json["role"] = message.role_;
            message_json["content"] = message.content_;
            message_json["timestamp"] = static_cast<int64_t>(message.timestamp_);
            data_array.append(message_json);
        }

        // 构建响应体
        Json::Value response_json;
        response_json["success"] = true;
        response_json["message"] = "get history messages success";
        response_json["data"] = data_array;

        // 序列化
        Json::StreamWriterBuilder writer_builder;
        std::string response_json_str = Json::writeString(writer_builder, response_json);

        response.status = 200; // 成功
        response.set_content(response_json_str, "application/json");
    }

    // 处理发送消息请求-全量返回
    void ChatServer::handle_send_message_request(const httplib::Request &request, httplib::Response &response)
    {
        // 获取请求参数
        Json::Value request_json;
        Json::Reader reader;
        if (!reader.parse(request.body, request_json))
        {
            std::string error_json_str = build_response("parse request body failed, json format error");
            response.status = 400; // 解析请求参数失败
            response.set_content(error_json_str, "application/json");
            return;
        }

        // 解析请求参数
        std::string session_id = request_json["session_id"].asString();
        std::string message = request_json["message"].asString();
        if (session_id.empty() || message.empty())
        {
            std::string error_json_str = build_response("session_id or message is empty");
            response.status = 400; // 解析请求参数失败
            response.set_content(error_json_str, "application/json");
            return;
        }

        // 发送消息
        std::string assistant_message = chat_sdk_->send_message(session_id, message);
        if (assistant_message.empty())
        {
            std::string error_json_str = build_response("Failed to send AI response message");
            response.status = 500; // 发送消息失败
            response.set_content(error_json_str, "application/json");
            return;
        }

        // 构造响应参数
        Json::Value data_json;
        data_json["session_id"] = session_id;
        data_json["response"] = assistant_message;

        // 构建响应体
        Json::Value response_json;
        response_json["success"] = true;
        response_json["message"] = "send message success";
        response_json["data"] = data_json;

        // 序列化
        Json::StreamWriterBuilder writer_builder;
        std::string response_json_str = Json::writeString(writer_builder, response_json);

        response.status = 200; // 成功
        response.set_content(response_json_str, "application/json");
    }

    // 处理发送消息请求-增量返回
    void ChatServer::handle_send_message_stream_request(const httplib::Request &request, httplib::Response &response)
    {
        // 获取请求参数
        Json::Value request_json;
        Json::Reader reader;
        if (!reader.parse(request.body, request_json))
        {
            std::string error_json_str = build_response("parse request body failed, json format error");
            response.status = 400; // 解析请求参数失败
            response.set_content(error_json_str, "application/json");
            return;
        }

        // 解析请求参数
        std::string session_id = request_json["session_id"].asString();
        std::string message = request_json["message"].asString();
        if (session_id.empty() || message.empty())
        {
            std::string error_json_str = build_response("session_id or message is empty");
            response.status = 400; // 解析请求参数失败
            response.set_content(error_json_str, "application/json");
            return;
        }

        // 准备流式响应
        response.status = 200;                                    // 成功
        response.set_header("Cache-Control", "no-cache");         // 不使用缓存，服务器立即将数据发送到网络
        response.set_header("Connection", "keep-alive");          // 保持连接，服务器不会关闭连接
        response.set_header("Access-Control-Allow-Origin", "*");  // 允许跨域请求
        response.set_header("Access-Control-Allow-Headers", "*"); // 允许所有请求头

        // set_chunked_content_provider：告诉服务器，响应内从不是一次性发送的，而是分多次逐步发送给客户端，一般用在实时生成响应内容 或者 流式数据传输场景
        response.set_chunked_content_provider("text/event-stream", [this, session_id, message](size_t offset, httplib::DataSink &data_sink) -> bool{
            auto write_chunk = [&](const std::string &chunk, bool last) {
                // 将chunk转换为SSE数据格式
                // Json::valueToQuotedString: 对chunk进行Json转换，目的防止chunk中包含一些特殊字符来破坏数据格式，比如：在chunk中包含了两个连续的换行，就会影响SSE数据格式
                std::string sse_data = "data: " + Json::valueToQuotedString(chunk.c_str()) + "\n\n";

                // 需要将模型返回的结果 chunk 发送给客户单
                data_sink.write(sse_data.c_str(), sse_data.size()); // 将数据写入响应流，即立即发送给客户单，该方法不会等待缓冲区满之后发送

                // 处理结束标记
                if (last) {
                    // 流向响应结束
                    std::string done_data = "data: [DONE]\n\n";
                    data_sink.write(done_data.c_str(), done_data.size());
                    data_sink.done(); // 表示流式响应结束
                    return false;    // 不再有后续数据
                }
                return true;
            };

            // 先给客户端发送一个空的数据块(占位符)，避免客户端长时间的等待
            if (!write_chunk("", false)) {
                return false;
            }

            // 发送消息流
            chat_sdk_->send_message_stream(session_id, message, write_chunk);

            return false; // 不再有后续数据
        });

        LOG_INFO("send message stream success");
    }

    // 设置HTTP路由规则
    void ChatServer::set_http_routes()
    {
        // 处理创建会话请求
        http_server_->Post("/api/session", [this](const httplib::Request &request, httplib::Response &response)
                           { handle_create_session_request(request, response); });

        // 处理获取会话列表请求
        http_server_->Get("/api/sessions", [this](const httplib::Request &request, httplib::Response &response)
                          { handle_get_session_lists_request(request, response); });

        // 处理获取模型列表请求
        http_server_->Get("/api/models", [this](const httplib::Request &request, httplib::Response &response)
                          { handle_get_model_lists_request(request, response); });

        // 处理删除会话请求
        http_server_->Delete("/api/session/(.*)", [this](const httplib::Request &request, httplib::Response &response)
                             { handle_delete_session_request(request, response); });

        // 处理获取历史消息请求
        http_server_->Get("/api/session/(.*)/history", [this](const httplib::Request &request, httplib::Response &response)
                          { handle_get_history_messages_request(request, response); });

        // 处理发送消息请求-全量返回
        http_server_->Post("/api/message", [this](const httplib::Request &request, httplib::Response &response)
                           { handle_send_message_request(request, response); });

        // 处理发送消息请求-增量返回
        http_server_->Post("/api/message/async", [this](const httplib::Request &request, httplib::Response &response)
                           { handle_send_message_stream_request(request, response); });
    }

} // namespace ai_chat_server