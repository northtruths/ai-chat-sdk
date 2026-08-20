# 附录：ChatSDK 使用手册

> 命名空间：`ai_chat_sdk`　语言标准：C++17　持久化：SQLite

---

## 1. ChatSDK 介绍

ChatSDK 是一款基于 C++ 语言实现的大模型接入库，目前支持：

- 已接入 **DeepSeek**（默认 `deepseek-v4-pro`）、**Gemini**（OpenAI 兼容格式）模型
- 支持 **Ollama** 本地接入（默认 `deepseek-r1:1.5b`）
- 支持多轮聊天，**全量消息**与**流式消息**两种响应
- 支持会话管理：创建会话、获取历史会话、获取历史会话消息、删除会话
- 使用 **SQLite** 对会话数据进行持久化存储，程序重启自动恢复

> 说明：`chatgpt` 分支已预留但尚未接入（仅打印日志）。

---

## 2. ChatSDK 获取

目录结构：

```
ai-chat-sdk/
├── CMakeLists.txt              # 构建入口（静态库 + 测试）
├── README.md
├── sdk/
│   ├── include/                # 对外头文件
│   │   ├── ChatSDK.hpp         # 顶层入口类
│   │   ├── BaseConfig.hpp      # 配置抽象基类
│   │   ├── DeepSeekConfig.hpp  # DeepSeek 配置
│   │   ├── OllamaConfig.hpp    # Ollama 配置
│   │   ├── GeminiConfig.hpp    # Gemini 配置
│   │   ├── LLMProvider.hpp     # Provider 抽象基类
│   │   ├── DeepSeekProvider.hpp / OllamaProvider.hpp / GeminiProvider.hpp
│   │   ├── LLMManager.hpp      # 模型调度
│   │   ├── SessionManager.hpp  # 会话管理
│   │   ├── DataManager.hpp     # SQLite 持久化
│   │   ├── Session.hpp         # Session / Message 数据结构
│   │   ├── Common.hpp          # 通用工具（状态码日志）
│   │   └── utils/              # 自研日志系统 mylog
│   └── src/                    # 实现源码
├── tests/                      # 测试程序
└── docs/                       # 文档
```

---

## 3. 使用说明

在程序中使用 ChatSDK 库时，主要通过 `ChatSDK` 类与库交互。

```
Header: #include <ai_chat_sdk/ChatSDK.hpp>
CMake:  target_link_libraries({ProjName} PRIVATE ai_chat_sdk)
```

### 3.1 public Functions

---

**`bool init_models(std::vector<std::shared_ptr<BaseConfig>>& configs)`**

功能：初始化所支持的模型
参数：configs - 所有支持的模型需配置的参数
返回值：初始化成功返回 true，否则返回 false

---

**`std::string create_session(const std::string& model_name)`**

功能：为 model_name 模型创建会话
参数：model_name - 模型系列名（`"deepseek"` / `"ollama"` / `"gemini"`，非具体版本号）
返回值：返回会话 Id

---

**`std::shared_ptr<Session> get_session(const std::string& session_id)`**

功能：获取 session_id 对应的会话信息
参数：session_id - 会话 Id
返回值：session_id 对应的会话信息（含全部历史消息）；不存在时返回空 shared_ptr

---

**`std::vector<std::string> get_session_list()`**

功能：获取所有会话信息（按更新时间降序）
返回值：返回所有会话 Id 列表

---

**`std::vector<std::string> get_available_models()`**

功能：获取当前已注册可用的所有模型
返回值：返回可用模型名称列表

---

**`bool delete_session(const std::string& session_id)`**

功能：删除 session_id 对应的会话信息（消息级联删除，同步数据库）
参数：session_id - 会话 Id
返回值：删除成功返回 true，否则返回 false

---

**`std::string send_message(const std::string session_id, const std::string& message)`**

功能：给大模型发送消息，大模型生成所有回复后一次性返回
参数：
　session_id - 会话 Id
　message - 给大模型发送的消息内容
返回值：返回大模型的完整回复（失败返回空字符串）

---

**`std::string send_message_stream(const std::string session_id, const std::string& message, std::function<void(const std::string&, bool)> callback)`**

功能：给大模型发送消息，大模型生成一点返回一点，即流式响应
参数：
　session_id - 会话 Id
　message - 给大模型发送的消息内容
　callback - 大模型返回消息用户处理回调函数，参数为（数据块内容, 是否为末尾数据）
返回值：返回大模型的完整回复（失败返回空字符串）

> 相关数据结构（`Session` / `Message`）请查看 `sdk/include/Session.hpp` 文件。

---

### 3.2 配置类说明

配置类均继承自 `BaseConfig`，通过 **方法** 设置参数（非公有成员赋值）：

```cpp
config->set_url(endpoint, path);   // 服务器地址 + 接口路径
config->set_api_key(api_key);      // 鉴权（Ollama 可省略）
config->set_model(model);          // 具体模型版本
config->set_temperature(temp);
config->set_max_tokens(tokens);
```

| 配置类 | series_name | 默认模型 | 默认 max_tokens | 默认 temperature |
|--------|-------------|----------|:---:|:---:|
| `DeepSeekConfig` | `deepseek` | `deepseek-v4-pro` | 4096 | 1.0 |
| `OllamaConfig` | `ollama` | `deepseek-r1:1.5b` | num_ctx=2048 | 0.7 |
| `GeminiConfig` | `gemini` | `gemini-3.5-flash` | 4096 | 1.0 |

> DeepSeek / Gemini 走 OpenAI 兼容格式，鉴权头为 `Authorization: Bearer <api_key>`；Ollama 本地无需 api_key。

---

### 3.3 快速上手

```cpp
#include <ai_chat_sdk/ChatSDK.hpp>
#include <ai_chat_sdk/DeepSeekConfig.hpp>
#include "utils/logger.hpp"
#include <iostream>
#include <memory>

using namespace ai_chat_sdk;

// 流式发送封装
void send_message_stream(ChatSDK& sdk, const std::string& session_id) {
    std::string message;
    std::getline(std::cin, message);
    sdk.send_message_stream(session_id, message,
        [](const std::string& response, bool done) {
            std::cout << "assistant消息: " << response << std::endl;
            if (done) {
                std::cout << "--------------接收消息完成--------------" << std::endl;
            }
        });
}

int main() {
    // 初始化日志
    mylog::Logger& logger = mylog::Logger::instance();
    logger.set_level(mylog::Level::INFO);
    logger.set_formatter(mylog::make_default_formatter());
    logger.set_transmitter(mylog::make_async_transmitter(4 * 1024 * 1024, 1000));
    logger.add_sink(mylog::make_file_sink("./logs", "aiChatServer.log"));

    ChatSDK sdk;

    // 初始化 deepseek 模型信息
    auto deepseek_config = std::make_shared<DeepSeekConfig>();
    deepseek_config->set_url("https://api.deepseek.com", "/chat/completions");
    deepseek_config->set_api_key(std::getenv("deepseek_apikey"));
    deepseek_config->set_temperature(0.7);
    deepseek_config->set_max_tokens(2048);
    deepseek_config->set_model("deepseek-chat");

    std::vector<std::shared_ptr<BaseConfig>> configs;
    configs.push_back(deepseek_config);

    // 初始化模型
    sdk.init_models(configs);

    std::cout << "--------------创建会话--------------" << std::endl;
    // 注意：传系列名 "deepseek"，不是具体版本号
    std::string session_id = sdk.create_session("deepseek");
    std::cout << "创建会话成功, 会话ID: " << session_id << std::endl;

    while (true) {
        std::cout << "-------1. send message  0. exit-----------------" << std::endl;
        int user_op = 1;
        std::cin >> user_op;
        if (user_op == 0) {
            break;
        }
        getchar();  // 吃掉换行符
        send_message_stream(sdk, session_id);
    }

    std::cout << "--------------程序退出--------------" << std::endl;
    return 0;
}
```

---

### 3.4 编译说明

进入sdk所在⽬录，然后执⾏如下命令：

1. mkdir build && cd build      # 创建build⽬录并进⼊该⽬录
2. cmake ..     # 生成makefile文件
3. cmake ai_chat_sdk    #编译ChatSDK生成  libai_chat_sdk.a 静态库
4. sudo make install    #安装静态库
静态库安装在：
/usr/local/lib
头⽂件安装位置：
/usr/local/include/ai_chat_sdk

---

## 4. 补充说明

- **失败约定**：所有发送接口失败时统一返回空字符串，失败原因通过日志（`mylog`）输出。
- **状态码**：HTTP 非 200 由 `Common.hpp` 中的 `log_error_code(int code)` 输出对应错误描述。
- **Gemini 400 排查**：请求体走 OpenAI 兼容格式（`messages`/`max_tokens`/`temperature`/`stream`），若报 400 且 body 相同的 Apifox 能通，重点排查 URL（`set_url` 的 endpoint 是否带 `https://`、path 是否为网关要求的完整路径）与鉴权头，而非请求体。
- **持久化**：`SessionManager` 构造时自动从 SQLite 加载历史会话；创建/发送/删除均实时写库。
