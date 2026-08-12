//  DataManager.hpp
//  功能：管理 SQLite 数据库，负责会话和消息的持久化存储

#pragma once

#include "Session.hpp"
#include <sqlite3.h>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <ctime>

namespace ai_chat_sdk
{
    class DataManager
    {
    public:
        // 构造函数：传入数据库文件路径
        DataManager(const std::string &db_path);

        // 析构函数：关闭数据库连接
        ~DataManager();

        // ========== 会话操作 ==========

        // 插入新会话
        bool insert_session(const Session &session);

        // 获取所有会话（自动加载每个会话的消息）
        std::vector<std::shared_ptr<Session>> get_all_sessions();

        // 获取单个会话（自动加载消息）
        std::shared_ptr<Session> get_session(const std::string &session_id);

        // 更新会话时间戳
        bool update_session_timestamp(const std::string &session_id, const std::time_t &time);

        // 删除会话（消息自动级联删除）
        bool delete_session(const std::string &session_id);

        // ========== 消息操作 ==========

        // 插入一条消息到指定会话
        bool insert_message(const std::string &session_id, const Message &msg);


    private:
        // 初始化数据库（创建表和索引）
        bool init_database();

        // 执行 SQL（无返回结果）
        bool execute_sql(const std::string &sql);
        
        // 获取指定会话的所有消息（按时间升序）
        std::vector<Message> get_messages(const std::string &session_id);
    
    private:
        sqlite3 *db_;                             // 数据库连接句柄
        std::string db_path_;                     // 数据库文件路径
        mutable std::mutex mutex_;                // 线程安全锁
        static std::atomic<int64_t> msg_counter_; // 消息计数器
    };

} // namespace ai_chat_sdk