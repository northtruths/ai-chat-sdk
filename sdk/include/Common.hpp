#pragma once

#include "utils/logger.hpp"

namespace ai_chat_sdk
{
    // 状态码详细报错
    void log_error_code(int code);

    struct ModelInfo
    {
        std::string model_name_;    // 模型名称
        std::string model_desc_;    // 模型描述
        std::string provider_;     // 模型提供者
        bool is_available_ = false; // 模型是否可用

        ModelInfo(const std::string &modelName = "", const std::string &modelDesc = "", const std::string &provider = "", const std::string &endpoint = "")
            : model_name_(modelName), model_desc_(modelDesc), provider_(provider)
        {
        }
    };
}
