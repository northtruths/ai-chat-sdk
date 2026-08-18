#include "Common.hpp"

namespace ai_chat_sdk{
    void log_error_code(int code)
    {
        mylog::Logger& logger = mylog::Logger::instance();
        switch (code)
        {
        case 0:
            LOG_INFO("状态码未被更改过");
        case 400:
            LOG_ERROR("信息发送失败！error：400；原因如下：请求格式有问题");
            break;
        case 401:
            LOG_ERROR("信息发送失败！error：401；原因如下：检查 API Key 是否正确或已过期");
            break;
        case 403:
            LOG_ERROR("信息发送失败！error：403；原因如下：账户权限不足，或 API Key 无权限访问该模型");
            break;
        case 404:
            LOG_ERROR("信息发送失败！error：404；原因如下：检查请求路径是否正确，或模型名称是否存在");
            break;
        case 429:
            LOG_ERROR("信息发送失败！error：429；原因如下：请求过于频繁，请稍后重试");
            break;
        case 500:
        case 502:
        case 503:
            LOG_ERROR("信息发送失败！error：503；原因如下：DeepSeek 服务器内部错误，请稍后重试");
            break;
        default:
            LOG_ERROR_STREAM() << "未知错误, 状态码为: " << code;
        }
    }
}