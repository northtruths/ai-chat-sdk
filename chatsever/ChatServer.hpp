#pragma once
#include <httplib.h>
#include <memory>
#include <ai_chat_sdk/ChatSDK.hpp>


namespace ai_chat_server {

// 服务器配置信息
struct ServerConfig {
    std::string host_ = "0.0.0.0";     // 服务器绑定ip
    int port_ = 8080;                  // 服务器绑定端口
    std::string log_level_ = "INFO";   // 日志级别

    // 模型需要的配置信息
    double temperature_ = 0.7;         // 温度参数
    int max_tokens_ = 1024;            // 最大token数

    // API Key
    std::string deepseek_api_key_;     // deepseek API Key
    std::string gemini_api_key_;       // gemini API Key
    std::string chatgpt_api_key_;      // chatGPT API Key

    // Ollama
    std::string ollama_model_name_;    // Ollama模型名称
    std::string ollama_model_desc_;    // Ollama模型描述
    std::string ollama_endpoint_;      // Ollama endpoint 地址
    std::string ollama_path_;          // Ollama path 路径
};


class ChatServer {
public:
    ChatServer(const ServerConfig& config);

    bool start();                      // 启动服务器
    void stop();                       // 停止服务器
    bool is_running() const;           // 是否正在运行

private:
    // 构造响应
    std::string build_response(const std::string& message, bool success = false);
    // 处理创建会话请求
    void handle_create_session_request(const httplib::Request& request, httplib::Response& response);
    // 处理获取会话列表请求
    void handle_get_session_lists_request(const httplib::Request& request, httplib::Response& response);
    // 处理获取模型列表请求
    void handle_get_model_lists_request(const httplib::Request& request, httplib::Response& response);
    // 处理删除会话请求
    void handle_delete_session_request(const httplib::Request& request, httplib::Response& response);
    // 处理获取历史消息请求
    void handle_get_history_messages_request(const httplib::Request& request, httplib::Response& response);
    // 处理发送消息请求-全量返回
    void handle_send_message_request(const httplib::Request& request, httplib::Response& response);
    // 处理发送消息请求-增量返回
    void handle_send_message_stream_request(const httplib::Request& request, httplib::Response& response);

    // 设置HTTP路由规则
    void set_http_routes();

private:
    ServerConfig config_;                              // 服务器配置信息
    std::unique_ptr<httplib::Server> http_server_;     // HTTP服务器
    std::shared_ptr<ai_chat_sdk::ChatSDK> chat_sdk_;   // 聊天SDK
    std::atomic<bool> is_running_ = false;             // 是否正在运行
};

} // namespace ai_chat_server