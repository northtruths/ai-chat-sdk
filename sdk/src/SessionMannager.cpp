#include "SessionManager.hpp"
#include "utils/logger.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace ai_chat_sdk
{

    std::atomic<int64_t> SessionManager::session_counter_{1};
    std::atomic<int64_t> SessionManager::message_counter_{1};

    SessionManager::SessionManager()
        : SessionManager("chat.db") // 委托构造
    {
    }

    SessionManager::SessionManager(const std::string &db_path)
        : data_manager_(std::make_unique<DataManager>(db_path))
    {
        // 从数据库加载会话
        load_from_database();
        LOG_INFO_STREAM() << "SessionManager 初始化完成, 数据库加载完成";
    }

    // 从数据库加载所有会话到内存
    void SessionManager::load_from_database()
    {
        LOG_DEBUG("开始从数据库加载");
        auto sessions = data_manager_->get_all_sessions();
        LOG_DEBUG("从数据库加载完成");
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto &session : sessions)
        {
            sessions_[session->id_] = session;
        }

        LOG_INFO_STREAM() << "从数据库加载了 " << sessions_.size() << " 个会话";
    }

    // 保存会话到数据库
    void SessionManager::save_session_to_db(const Session &session)
    {
        data_manager_->insert_session(session);
    }

    // 保存消息到数据库
    void SessionManager::save_message_to_db(const std::string &session_id, const Message &msg)
    {
        data_manager_->insert_message(session_id, msg);
    }

    // 数据库中删除会话
    bool SessionManager::delete_session_to_db(const std::string &session_id)
    {
        bool ok = data_manager_->delete_session(session_id);
        if (!ok)
        {
            LOG_WARN("删除会话失败");
            return false;
        }
        return true;
    }

    // 更新数据库会话时间戳
    bool SessionManager::update_session_to_db(const std::string &session_id, const std::time_t &time){
        return data_manager_->update_session_timestamp(session_id, time);
    }

    // 生成 id
    std::string SessionManager::generate_id(const std::string &prefix)
    {
        int64_t counter = (prefix == "session_")
                              ? session_counter_.fetch_add(1)
                              : message_counter_.fetch_add(1);

        std::time_t now = std::time(nullptr);
        std::ostringstream oss;
        oss << prefix << now << "_" << std::setfill('0') << std::setw(8) << counter;
        return oss.str();
    }

    // 创建会话
    std::string SessionManager::create_session(const std::string &model_name)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto session = std::make_shared<Session>(model_name);
        session->id_ = generate_id("session_");
        sessions_[session->id_] = session;

        save_session_to_db(*session);
        LOG_INFO_STREAM() << "创建 " << model_name << " 模型会话 {" << session->id_ << "} ";
        return session->id_;
    }

    // 获取会话
    std::shared_ptr<Session> SessionManager::get_session(const std::string &session_id) const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = sessions_.find(session_id);
        if (it == sessions_.end())
        {
            LOG_WARN_STREAM() << "会话 {" << session_id << "} 不存在";
            return nullptr;
        }
        return it->second;
    }

    // 添加消息，传入的消息类需要带 role 和 content
    bool SessionManager::add_message(const std::string &session_id, const std::string &role, const std::string &content)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto cur_session_hash = sessions_.find(session_id);
        if (cur_session_hash == sessions_.end())
        {
            LOG_WARN_STREAM() << "添加消息失败：会话 {" << session_id << "} 不存在";
            return false;
        }

        // 给消息添加 id 和时间
        Message msg;
        msg.id_ = generate_id("msg_");
        msg.role_ = role;
        msg.content_ = content;
        msg.timestamp_ = std::time(nullptr);

        cur_session_hash->second->messages_.push_back(msg);
        cur_session_hash->second->updated_at_ = std::time(nullptr);
        save_message_to_db(session_id, msg);

        return true;
    }

    //  获取历史消息
    std::vector<Message> SessionManager::get_history(const std::string &session_id) const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto cur_session_hash = sessions_.find(session_id);
        if (cur_session_hash == sessions_.end())
        {
            LOG_WARN_STREAM() << "获取历史消息失败：会话 {" << session_id << "} 不存在";
            return {};
        }
        return cur_session_hash->second->messages_;
    }

    // 获取会话列表 (返回 session id)
    std::vector<std::string> SessionManager::get_session_list() const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<std::pair<std::time_t, std::string>> temp;
        temp.reserve(sessions_.size());

        for (const auto &pair : sessions_)
        {
            if(!pair.second){
                LOG_ERROR("(SessionManager) 会话数据错误，包含了错误/无效会话");
                continue;
            }
            temp.emplace_back(pair.second->updated_at_, pair.first);
            LOG_DEBUG_STREAM() << pair.first << " 会话已获取";
        }

        // 按更新时间降序
        std::sort(temp.begin(), temp.end(),
                  [](const auto &a, const auto &b)
                  { return a.first > b.first; });

        std::vector<std::string> result;
        result.reserve(temp.size());
        for (const auto &item : temp)
        {
            result.push_back(item.second);
        }

        return result;
    }

    // 删除会话
    bool SessionManager::delete_session(const std::string &session_id)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto cur_session_hash = sessions_.find(session_id);
        if (cur_session_hash == sessions_.end())
        {
            LOG_WARN_STREAM() << "删除失败：会话 {" << session_id << "} 不存在";
            return false;
        }

        sessions_.erase(cur_session_hash);
        delete_session_to_db(session_id);

        LOG_INFO_STREAM() << "会话 {" << session_id << "} 已删除";
        return true;
    }

    // 更新时间戳
    void SessionManager::update_timestamp(const std::string &session_id)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto cur_session_hash = sessions_.find(session_id);
        if (cur_session_hash != sessions_.end())
        {
            std::time_t cur_time = std::time(nullptr);
            cur_session_hash->second->updated_at_ = cur_time;
           bool ok = update_session_to_db(session_id, cur_time);
           if(!ok){
            LOG_ERROR_STREAM() << "数据库更新时间戳失败, 会话 id 为: " << session_id;  
           }
        }
    }

    // 清空所有会话
    void SessionManager::clear_all()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for(auto& cur_session_hash : sessions_){
            std::string cur_session_id = cur_session_hash.first;
            delete_session(cur_session_id);
        }
        sessions_.clear();
        LOG_INFO("所有会话已清空");
    }

    // 获取会话总数
    size_t SessionManager::get_count() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return sessions_.size();
    }

    // 设置会话简介
    bool SessionManager::set_session_desc(const std::string &session_id, const std::string &desc){
        std::lock_guard<std::mutex> lock(mutex_);

        auto cur_session_hash = sessions_.find(session_id);
        if (cur_session_hash == sessions_.end())
        {
            LOG_WARN_STREAM() << "设置简介失败：会话 {" << session_id << "} 不存在";
            return false;
        }

        cur_session_hash->second->session_desc_ = desc;
        data_manager_->set_session_desc(session_id, desc);

        LOG_INFO_STREAM() << "会话 {" << session_id << "} 已修改简介为: " << desc;
        return true;
    }

}