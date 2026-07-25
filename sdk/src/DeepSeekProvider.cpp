
#include "Common.hpp"
#include "DeepSeekProvider.hpp"
#include "utils/logger.hpp"
#include "Message.hpp"
#include <string>
#include <jsoncpp/json/json.h>

using namespace log;
namespace ai_chat_sdk
{
    // 初始化模型
    bool DeepSeekProvider::init_model(const Config cf)
    {
        config_ = cf;
        if (config_.get("name") == std::string())
        {
            LOG_ERROR("deepseek初始化模型失败：未指定模型名称");
            return false;
        }
        else
        {
            name_ = config_.get("name");
        }

        if (cf.get("endpoint") == std::string())
        {
            LOG_ERROR("deepseek初始化模型失败，未提供模型url");
            return false;
        }
        else
        {
            endpoint_ = config_.get("endpoint");
        }
        is_available_ = true;
        LOG_INFO("deepseek模型初始化成功");
        return true;
    }
    // 检测模型是否有效
    bool DeepSeekProvider::is_available() const
    {
        
        return is_available_;
    }

    // 获取模型名称
    std::string DeepSeekProvider::get_model_name() const
    {
        return name_;
    }
    // 获取模型描述
    std::string DeepSeekProvider::get_model_desc() const
    {
        // 若没有设置描述则为空
        return config_.get("desc");
    }

    // 设置/更改配置参数
    bool DeepSeekProvider::set_model_params(const Config cf){
        if(cf.get("name") == std::string() || cf.get("api_key") == std::string()){
            LOG_WARN("配置参数没有包含必要的 'name' 或 'api_key' ");
            return false;
        }
        config_ = cf;
        return true;
    }

    // 获取模型配置信息
    Config &DeepSeekProvider::get_model_setting()
    {
        return config_;
    }

    // 全量式发送信息
    std::string DeepSeekProvider::send_message()
    {
        
    }

    // 流式发送信息
    std::string  DeepSeekProvider::send_meesage_stream()
    {
    }
}