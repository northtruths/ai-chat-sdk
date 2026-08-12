//  DataManager.cpp

#include "DataManager.hpp"
#include "utils/logger.hpp"
#include <sstream>
#include <iomanip>

using namespace mylog;

namespace ai_chat_sdk
{
    //  构造函数
    DataManager::DataManager(const std::string &db_path)
        : db_path_(db_path), db_(nullptr)
    {
        int rc = sqlite3_open(db_path_.c_str(), &db_);
        if (rc != SQLITE_OK)
        {
            LOG_ERROR_STREAM() << "（数据库）打开数据库失败: " << sqlite3_errmsg(db_);
            return;
        }
        LOG_INFO_STREAM() << "（数据库）数据库打开成功: " << db_path_;

        if (!init_database())
        {
            LOG_ERROR_STREAM() << "（数据库）初始化数据库失败";
            sqlite3_close(db_);
            db_ = nullptr;
        }
    }
 
    //  析构函数
    DataManager::~DataManager()
    {
        if (db_)
        {
            // 处理数据残留
            execute_sql("PRAGMA wal_checkpoint;");
            sqlite3_close(db_);
            LOG_INFO_STREAM() << "（数据库）数据库已关闭";
        }
    }

    //  执行 SQL（无返回结果）
    bool DataManager::execute_sql(const std::string &sql)
    {
        char *errmsg = nullptr;
        int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errmsg);

        if (rc != SQLITE_OK)
        {
            LOG_ERROR_STREAM() << "（数据库）SQL执行失败: " << errmsg;
            sqlite3_free(errmsg);
            return false;
        }
        return true;
    }

    //  初始化数据库：创建表和索引
    bool DataManager::init_database()
    {
        // 开启外键
        execute_sql("PRAGMA foreign_keys = ON;");
        // 开启WAL
        execute_sql("PRAGMA journal_mode=WAL;");
        std::string create_sessions = R"(
        CREATE TABLE IF NOT EXISTS sessions (
            id TEXT PRIMARY KEY,
            model_name TEXT NOT NULL,
            created_at INTEGER NOT NULL,
            updated_at INTEGER NOT NULL,
            desc TEXT NOT NULL
        )
    )";
        if (!execute_sql(create_sessions))
            return false;

        std::string create_messages = R"(
        CREATE TABLE IF NOT EXISTS messages (
            id TEXT PRIMARY KEY,
            session_id TEXT NOT NULL,
            role TEXT NOT NULL,
            content TEXT NOT NULL,
            timestamp INTEGER NOT NULL,
            FOREIGN KEY (session_id) REFERENCES sessions(id) ON DELETE CASCADE
        )
    )";
        if (!execute_sql(create_messages))
            return false;

        std::string create_index = R"(
        CREATE INDEX IF NOT EXISTS idx_messages_session_id ON messages(session_id)
    )";
        if (!execute_sql(create_index))
            return false;

        LOG_INFO_STREAM() << "（数据库）数据库表初始化成功";
        return true;
    }

    //  插入会话
    bool DataManager::insert_session(const Session &session)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const char *sql = "INSERT INTO sessions (id, model_name, created_at, updated_at, desc) VALUES (?, ?, ?, ?, ?)";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            LOG_ERROR_STREAM() << "（数据库）准备SQL失败: " << sqlite3_errmsg(db_);
            return false;
        }

        sqlite3_bind_text(stmt, 1, session.id_.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, session.model_name_.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, session.created_at_);
        sqlite3_bind_int64(stmt, 4, session.updated_at_);
        sqlite3_bind_text(stmt, 5, session.session_desc_.c_str(), -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE)
        {
            LOG_ERROR_STREAM() << "（数据库）插入会话失败: " << sqlite3_errmsg(db_);
            return false;
        }
        LOG_DEBUG_STREAM() << "（数据库）插入会话: " << session.id_;
        return true;
    }

    //  插入消息
    bool DataManager::insert_message(const std::string &session_id, const Message &msg)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        const char *sql = "INSERT INTO messages (id, session_id, role, content, timestamp) VALUES (?, ?, ?, ?, ?)";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            LOG_ERROR_STREAM() << "（数据库）准备SQL失败: " << sqlite3_errmsg(db_);
            return false;
        }

        sqlite3_bind_text(stmt, 1, msg.id_.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, session_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, msg.role_.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, msg.content_.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 5, msg.timestamp_);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE)
        {
            LOG_ERROR_STREAM() << "（数据库）插入消息失败: " << sqlite3_errmsg(db_);
            return false;
        }
        LOG_DEBUG_STREAM() << "（数据库）插入消息: " << msg.id_ << " -> " << session_id;
        return true;
    }

    //  获取指定会话的所有消息（按时间升序）
    std::vector<Message> DataManager::get_messages(const std::string &session_id)
    {
        std::vector<Message> result;

        const char *sql = "SELECT id, role, content, timestamp FROM messages WHERE session_id = ? ORDER BY timestamp ASC";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            LOG_ERROR_STREAM() << "（数据库）准备SQL失败: " << sqlite3_errmsg(db_);
            return result;
        }

        sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_TRANSIENT);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            Message msg;
            msg.id_ = (const char *)sqlite3_column_text(stmt, 0);
            msg.role_ = (const char *)sqlite3_column_text(stmt, 1);
            msg.content_ = (const char *)sqlite3_column_text(stmt, 2);
            msg.timestamp_ = sqlite3_column_int64(stmt, 3);
            result.push_back(msg);
        }

        sqlite3_finalize(stmt);
        LOG_DEBUG_STREAM() << "（数据库）获取消息 " << result.size() << " 条 (会话: " << session_id << ")";
        return result;
    }

    //  获取所有会话（自动加载每个会话的消息）
    std::vector<std::shared_ptr<Session>> DataManager::get_all_sessions()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::shared_ptr<Session>> result;

        const char *sql = "SELECT id, model_name, created_at, updated_at, desc FROM sessions ORDER BY updated_at DESC";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            LOG_ERROR_STREAM() << "（数据库）准备SQL失败: " << sqlite3_errmsg(db_);
            return result;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            auto session = std::make_shared<Session>();
            session->id_ = (const char *)sqlite3_column_text(stmt, 0);
            session->model_name_ = (const char *)sqlite3_column_text(stmt, 1);
            session->created_at_ = sqlite3_column_int64(stmt, 2);
            session->updated_at_ = sqlite3_column_int64(stmt, 3);
            session->session_desc_ = (const char *)sqlite3_column_text(stmt, 4);

            session->messages_ = get_messages(session->id_);

            result.push_back(session);
        }

        sqlite3_finalize(stmt);
        LOG_DEBUG_STREAM() << "（数据库）加载了 " << result.size() << " 个会话";
        return result;
    }

    //  获取单个会话
    std::shared_ptr<Session> DataManager::get_session(const std::string &session_id)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        const char *sql = "SELECT id, model_name, created_at, updated_at, desc FROM sessions WHERE id = ?";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            LOG_ERROR_STREAM() << "（数据库）准备SQL失败: " << sqlite3_errmsg(db_);
            return nullptr;
        }

        sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_ROW)
        {
            sqlite3_finalize(stmt);
            return nullptr;
        }

        auto session = std::make_shared<Session>();
        session->id_ = (const char *)sqlite3_column_text(stmt, 0);
        session->model_name_ = (const char *)sqlite3_column_text(stmt, 1);
        session->created_at_ = sqlite3_column_int64(stmt, 2);
        session->updated_at_ = sqlite3_column_int64(stmt, 3);
        session->session_desc_ = (const char *)sqlite3_column_text(stmt, 4);

        sqlite3_finalize(stmt);

        session->messages_ = get_messages(session_id);

        LOG_DEBUG_STREAM() << "（数据库）获取会话: " << session_id;
        return session;
    }

    //  更新会话时间戳 
    bool DataManager::update_session_timestamp(const std::string &session_id, const std::time_t &time)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        const char *sql = "UPDATE sessions SET updated_at = ? WHERE id = ?";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            LOG_ERROR_STREAM() << "（数据库）准备SQL失败: " << sqlite3_errmsg(db_);
            return false;
        }

        sqlite3_bind_int64(stmt, 1, time);
        sqlite3_bind_text(stmt, 2, session_id.c_str(), -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE)
        {
            LOG_ERROR_STREAM() << "（数据库）更新会话时间戳失败: " << sqlite3_errmsg(db_);
            return false;
        }
        LOG_DEBUG_STREAM() << "（数据库）更新会话时间戳: " << session_id;
        return true;
    }

    //  删除会话（ON DELETE CASCADE 自动删除消息）
    bool DataManager::delete_session(const std::string &session_id)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        const char *sql = "DELETE FROM sessions WHERE id = ?";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            LOG_ERROR_STREAM() << "（数据库）准备SQL失败: " << sqlite3_errmsg(db_);
            return false;
        }

        sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE)
        {
            LOG_ERROR_STREAM() << "（数据库）删除会话失败: " << sqlite3_errmsg(db_);
            return false;
        }
        LOG_DEBUG_STREAM() << "（数据库）删除会话: " << session_id;
        return true;
    }

    //  设置会话简介
    bool DataManager::set_session_desc(const std::string &session_id, const std::string &desc)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        const char *sql = "UPDATE sessions SET desc = ? WHERE id = ?";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            LOG_ERROR_STREAM() << "（数据库）准备SQL失败: " << sqlite3_errmsg(db_);
            return false;
        }

        sqlite3_bind_text(stmt, 1, desc.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, session_id.c_str(), -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE)
        {
            LOG_ERROR_STREAM() << "（数据库）更新会话简介失败: " << sqlite3_errmsg(db_);
            return false;
        }
        LOG_DEBUG_STREAM() << "（数据库）" << session_id << "更新会话简介为: " << desc;
        return true;
    }

}// namespace ai_chat_sdk