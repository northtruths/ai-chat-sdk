# AI Chat SDK 项目使用说明

## 1. 项目简介

本项目由三部分组成：

- `sdk/`：C++ 大模型接入 SDK，生成静态库 `libai_chat_sdk.a`
- `chatsever/`：基于 httplib 的 HTTP 服务端
- `www/`：ChatServer 使用的前端静态网页

当前支持的模型 Provider：

- `deepseek`：DeepSeek 云端模型
- `chatgpt`：ChatGPT 云端模型
- `gemini`：Gemini 云端模型
- `ollama`：Ollama 本地模型

> 注意：模型列表中的 `name` 是具体模型版本，例如 `deepseek-v4-pro`；创建会话时应传 `provider`，例如 `deepseek`，不能直接传具体版本名。

## 2. 目录结构

```text
ai-chat-sdk/
├── CMakeLists.txt                 # 项目旧版/总入口，实际服务器使用独立 CMake
├── README.md                      # 简介
├── ChatServer.conf                # ChatServer 的 gflags 配置文件
├── Key.json                       # 云端 API Key 配置文件
├── config.json                    # 旧的综合配置示例，当前 ChatServer 不直接读取
├── chat.db                        # SQLite 数据库，运行后自动生成
├── logs/                          # 日志目录，运行后自动生成
├── bin/
│   └── AIChatServer               # ChatServer 可执行文件
├── sdk/
│   ├── CMakeLists.txt             # SDK 独立 CMake
│   ├── include/                   # SDK 对外头文件
│   ├── src/                       # SDK 实现源码
│   ├── tests/                     # SDK 测试源码
│   └── build/                     # SDK 构建目录
├── chatsever/
│   ├── CMakeLists.txt             # ChatServer 独立 CMake
│   ├── main.cpp                   # 服务端入口和 gflags 定义
│   ├── ChatServer.hpp             # 服务端类声明
│   ├── ChatServer.cpp             # HTTP 路由和请求处理
│   └── build/                     # ChatServer 构建目录
├── www/
│   ├── index.html                 # 前端入口
│   ├── css/style.css              # 前端样式
│   └── js/app.js                  # 前端逻辑
└── docs/                          # 项目文档
```

## 3. 依赖环境

建议环境：

- Linux
- CMake 3.16 或更高版本
- C++17 编译器
- OpenSSL
- JsonCpp
- SQLite3
- gflags
- GoogleTest（仅构建 SDK 测试时需要）
- cpp-httplib 头文件
- Ollama（使用本地模型时需要）

Ubuntu/Debian 可按需安装：

```bash
sudo apt update
sudo apt install build-essential cmake libssl-dev libjsoncpp-dev \
    libsqlite3-dev libgflags-dev libgtest-dev curl
```

`cpp-httplib` 和 `ai_chat_sdk` 的安装位置由当前 CMake 查找：

```text
/usr/local/include/ai_chat_sdk/
/usr/local/lib/libai_chat_sdk.a
/usr/local/include/httplib.h
```

## 4. SDK 构建与安装

SDK 是独立 CMake 工程。进入 `sdk/` 目录执行：

```bash
cd ~/Projects/ai-chat-sdk/sdk
cmake -S . -B build
cmake --build build -j2
sudo cmake --install build
```

构建静态库：

```text
sdk/build/libai_chat_sdk.a
```

安装后通常得到：

```text
/usr/local/lib/libai_chat_sdk.a
/usr/local/include/ai_chat_sdk/*.hpp
```

只构建 SDK 测试目标：

```bash
cmake --build build --target test_chat -j2
```

## 5. ChatServer 构建

ChatServer 的 CMake 位于 `chatsever/CMakeLists.txt`，它链接已安装的 SDK，因此修改 SDK 后需要重新安装 SDK，再构建 ChatServer：

```bash
cd ~/Projects/ai-chat-sdk/chatsever
cmake -S . -B build
cmake --build build -j2
```

可执行文件输出到项目根目录：

```text
~/Projects/ai-chat-sdk/bin/AIChatServer
```

如果 CMake 仍使用旧缓存，可以重新生成构建目录：

```bash
rm -rf ~/Projects/ai-chat-sdk/chatsever/build
cmake -S ~/Projects/ai-chat-sdk/chatsever \
      -B ~/Projects/ai-chat-sdk/chatsever/build
cmake --build ~/Projects/ai-chat-sdk/chatsever/build -j2
```

## 6. 配置文件说明

### 6.1 ChatServer.conf

位置：

```text
ai-chat-sdk/ChatServer.conf
```

这是 gflags 的 flagfile，不是 JSON 文件。每个配置项必须使用 `--参数=值` 格式：

```ini
--host=0.0.0.0
--port=8080
--log_level=INFO
--temperature=0.7
--max_tokens=2048
--key_file=Key.json
--db_path=chat.db
--ollama_model_name=deepseek-r1:1.5b
--ollama_model_desc=Ollama 本地运行的 DeepSeek R1 轻量模型
--ollama_endpoint=http://localhost:11434
--ollama_path=/api/chat
```

当前支持的 gflags：

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `host` | `0.0.0.0` | HTTP 监听地址 |
| `port` | `8080` | HTTP 监听端口 |
| `log_level` | `INFO` | 日志级别 |
| `temperature` | `0.7` | 模型温度 |
| `max_tokens` | `2048` | 最大 token 或上下文参数 |
| `config_file` | `ChatServer.conf` | gflags 配置文件路径 |
| `key_file` | `Key.json` | API Key 文件路径 |
| `db_path` | `chat.db` | 数据库路径；当前服务端 SDK 实际仍使用默认的 `chat.db` |
| `ollama_model_name` | 空 | Ollama 具体模型名 |
| `ollama_model_desc` | 空 | Ollama 模型描述 |
| `ollama_endpoint` | 空 | Ollama 服务地址 |
| `ollama_path` | `/api/chat` | Ollama API 路径 |

命令行参数会覆盖配置文件中的同名参数，例如：

```bash
./bin/AIChatServer --port=9000
```

### 6.2 Key.json

位置：

```text
ai-chat-sdk/Key.json
```

这是 JsonCpp 格式的云端 API Key 文件：

```json
{
    "deepseek_api_key": "替换为你的 DeepSeek Key",
    "chatgpt_api_key": "替换为你的 ChatGPT Key",
    "gemini_api_key": "替换为你的 Gemini Key"
}
```

Ollama 不需要 API Key。可以只配置完整的 Ollama 模型名、描述和端点，也可以配置一个或多个云端 API Key。

不要将真实 Key 提交到 Git 仓库。建议设置权限：

```bash
chmod 600 Key.json
```

### 6.3 config.json

位置：

```text
ai-chat-sdk/config.json
```

这是早期的综合配置示例，包含模型 endpoint、path、model 和 key 等字段。当前 ChatServer 读取的是 `Key.json` 和 `ChatServer.conf`，不会直接读取这个文件。为避免混淆，实际启动配置应以这两个文件为准。

### 6.4 ChatServer.conf 与 Key.json 的相对路径

程序中的相对路径相对于“当前工作目录”，不是相对于可执行文件所在目录。因此推荐固定从项目根目录启动：

```bash
cd ~/Projects/ai-chat-sdk
./bin/AIChatServer
```

此时以下路径有效：

```text
ChatServer.conf -> ai-chat-sdk/ChatServer.conf
Key.json       -> ai-chat-sdk/Key.json
www/           -> ai-chat-sdk/www/
logs/          -> ai-chat-sdk/logs/
chat.db        -> ai-chat-sdk/chat.db
```

如果从 `bin/` 目录启动，需要改用 `../` 路径：

```bash
cd ~/Projects/ai-chat-sdk/bin
./AIChatServer --config_file=../ChatServer.conf --key_file=../Key.json
```

## 7. Ollama 配置

安装并启动 Ollama 后下载默认模型：

```bash
ollama serve
ollama pull deepseek-r1:1.5b
```

在根目录 `ChatServer.conf` 中配置：

```ini
--ollama_model_name=deepseek-r1:1.5b
--ollama_model_desc=Ollama 本地运行的 DeepSeek R1 轻量模型
--ollama_endpoint=http://localhost:11434
--ollama_path=/api/chat
```

启动日志应出现：

```text
Ollama 模型: deepseek-r1:1.5b
Ollama 端点: http://localhost:11434
```

发送请求时，SDK 会将请求提交到：

```text
http://localhost:11434/api/chat
```

## 8. 启动服务器

推荐方式：

```bash
cd ~/Projects/ai-chat-sdk
./bin/AIChatServer
```

成功启动后访问：

```text
http://127.0.0.1:8080/
```

后台运行并保存输出：

```bash
cd ~/Projects/ai-chat-sdk
nohup ./bin/AIChatServer > server.out 2>&1 &
```

停止服务器：

```bash
pkill -f './bin/AIChatServer'
```

检查进程和端口：

```bash
pgrep -a AIChatServer
ss -ltnp | grep ':8080'
```

## 9. 前端页面

前端文件位于根目录 `www/`，服务端通过以下代码挂载静态目录：

```cpp
http_server_->set_mount_point("/", "./www");
```

因此必须从项目根目录启动，或者保证当前工作目录下存在 `www/`。

浏览器访问：

```text
http://127.0.0.1:8080/
```

前端使用同源相对 API 路径，不需要单独启动前端开发服务器。修改 `www/` 文件后刷新浏览器即可；遇到缓存时使用 `Ctrl+F5`。

## 10. HTTP API

### 获取模型列表

```bash
curl http://127.0.0.1:8080/api/models
```

返回数据中的 `name` 是具体模型版本，`provider` 是创建会话时使用的模型系列名：

```json
{
    "name": "deepseek-v4-pro",
    "provider": "deepseek"
}
```

### 创建会话

传 Provider 系列名：

```bash
curl -X POST http://127.0.0.1:8080/api/session \
    -H 'Content-Type: application/json' \
    -d '{"model":"ollama"}'
```

可用值：

```text
deepseek
chatgpt
gemini
ollama
```

不要传：

```text
deepseek-v4-pro
deepseek-r1:1.5b
gemini-3.5-flash
```

### 获取会话列表

```bash
curl http://127.0.0.1:8080/api/sessions
```

### 获取历史消息

```bash
curl http://127.0.0.1:8080/api/session/<session_id>/history
```

### 发送完整消息

```bash
curl -X POST http://127.0.0.1:8080/api/message \
    -H 'Content-Type: application/json' \
    -d '{"session_id":"<session_id>","message":"你好"}'
```

### 发送流式消息

```bash
curl -N -X POST http://127.0.0.1:8080/api/message/async \
    -H 'Content-Type: application/json' \
    -d '{"session_id":"<session_id>","message":"你好"}'
```

### 删除会话

```bash
curl -X DELETE http://127.0.0.1:8080/api/session/<session_id>
```

## 11. 日志与数据库

运行时文件默认位于项目根目录：

```text
ai-chat-sdk/logs/AIChatServer.log
ai-chat-sdk/chat.db
```

实时查看日志：

```bash
tail -f logs/AIChatServer.log
```

数据库使用 SQLite，服务启动时会自动创建数据库表，并在重启后恢复会话。

## 12. CMake 清理目标

在 `chatsever/build/` 中可使用：

```bash
make clean_logs   # 删除 ChatServer 日志
make clean_db     # 删除 chatsever 目录下数据库（当前运行数据库需注意实际工作目录）
make clean_data   # 删除日志和数据库
make clean_build  # 删除 ChatServer 构建目录
make clean_all    # 删除构建目录、运行数据和根目录 bin
```

SDK 中可使用：

```bash
cd sdk/build
make cleanlog
make cleanall
```

## 13. 常见问题

### 13.1 启动后立即退出

通常是当前工作目录下找不到 `ChatServer.conf` 或 `Key.json`。先确认：

```bash
pwd
ls -l ChatServer.conf Key.json www
```

推荐回到项目根目录启动。

### 13.2 模型找不到

创建会话必须传 Provider 系列名：

```json
{"model":"ollama"}
```

而不是具体版本名：

```json
{"model":"deepseek-r1:1.5b"}
```

### 13.3 前端访问 404

确认服务器启动时的当前目录包含 `www/`：

```bash
cd ~/Projects/ai-chat-sdk
./bin/AIChatServer
```

### 13.4 SDK 修改后 ChatServer 没有变化

ChatServer 链接的是 `/usr/local/lib/libai_chat_sdk.a`。SDK 修改后需要重新安装：

```bash
cd sdk
cmake --build build -j2
sudo cmake --install build
cd ../chatsever
cmake --build build -j2
```

### 13.5 端口被占用

查看占用进程：

```bash
ss -ltnp | grep ':8080'
```

临时改端口：

```bash
./bin/AIChatServer --port=8081
```

## 14. 最小启动流程

```bash
cd ~/Projects/ai-chat-sdk

# 1. 构建并安装 SDK
cmake -S sdk -B sdk/build
cmake --build sdk/build -j2
sudo cmake --install sdk/build

# 2. 构建 ChatServer
cmake -S chatsever -B chatsever/build
cmake --build chatsever/build -j2

# 3. 启动 Ollama（使用 Ollama 时）
ollama serve

# 4. 启动 ChatServer
./bin/AIChatServer

# 5. 浏览器访问
# http://127.0.0.1:8080/
```
