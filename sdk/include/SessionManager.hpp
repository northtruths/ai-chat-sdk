#pragma once

#include "Session.hpp"
#include "DataManager.hpp"
#include "LLMManager.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <ctime>
#include <atomic>

namespace ai_chat_sdk
{
    class SessionManager
    {
    public:
        // 构造，初始化会话并且会将数据库信息同步下来
        SessionManager();
        SessionManager(const std::string &db_path);

        // 创建会话(同步数据库)
        std::string create_session(const std::string &model_name);

        // 获取会话（返回 shared_ptr，安全共享）
        std::shared_ptr<Session> get_session(const std::string &session_id) const;

        // 添加消息到会话(会同步数据库)
        bool add_message(const std::string &session_id, const std::string &role, const std::string &content);

        // 获取会话历史消息
        std::vector<Message> get_history(const std::string &session_id) const;

        // 获取所有会话列表（按更新时间降序）
        std::vector<std::string> get_session_list() const;

        // 删除会话(同步数据库)
        bool delete_session(const std::string &session_id);

        // 清空所有会话(同步数据库)
        void clear_all();

        // 获取会话总数
        size_t get_count() const;

        // 设置会话简介
        bool set_session_desc(const std::string &session_id, const std::string &desc);

    private:
        // 更新会话时间戳(同步数据库)
        void update_timestamp(const std::string &session_id);

        // 生成唯一 id，由前缀+时间错+计数组成（例session_1699999999_00000001）
        std::string generate_id(const std::string &prefix);

        // 从数据库加载所有会话到内存
        void load_from_database();

        // 保存会话到数据库
        void save_session_to_db(const Session &session);

        // 保存消息到数据库
        void save_message_to_db(const std::string &session_id, const Message &msg);

        // 数据库中删除会话
        bool delete_session_to_db(const std::string& session_id);

        // 更新数据库会话时间戳
        bool update_session_to_db(const std::string &session_id, const std::time_t &time);

    private:
        // 会话存储
        std::unordered_map<std::string, std::shared_ptr<Session>> sessions_;

        // 线程安全
        mutable std::mutex mutex_;

        // 计数器
        static std::atomic<int64_t> session_counter_;
        static std::atomic<int64_t> message_counter_;

        // 数据库管理
        std::unique_ptr<DataManager> data_manager_;
    };
}
