// Session.hpp
#pragma once
#include <string>
#include <vector>
#include <ctime>

namespace ai_chat_sdk
{

    // id 由前缀+时间错+计数组成（例 session_1699999999_00000001）
    struct Message
    {
        std::string id_;        // 消息唯一 id
        std::string role_;      // "user" / "assistant"
        std::string content_;   // 消息内容
        std::time_t timestamp_; // 发送时间
    };

    struct Session
    {
        std::string id_;                // 会话唯一 id
        std::string model_name_;        // 使用的模型名
        std::vector<Message> messages_; // 所有历史消息
        std::time_t created_at_;        // 创建时间
        std::time_t updated_at_;        // 最后更新时间
        std::string session_desc_;      //会话简介

        Session()
            : created_at_(std::time(nullptr)), updated_at_(std::time(nullptr)), session_desc_("新会话") {}
        Session(const std::string &model)
            : model_name_(model), created_at_(std::time(nullptr)), updated_at_(std::time(nullptr)), session_desc_("新会话") {}
    };

} // namespace ai_chat_sdk