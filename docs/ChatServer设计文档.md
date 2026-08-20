# AI Chat Server API 接口文档

## 1. 基础信息

| 项目           | 说明                    |
| ------------ | --------------------- |
| **Base URL** | `http://服务器IP:端口`     |
| **默认地址**     | `http://0.0.0.0:8080` |
| **响应格式**     | JSON                  |
| **字符编码**     | UTF-8                 |

> 以下接口均基于 HTTP 协议，除流式消息接口外，普通接口均返回 JSON 格式数据。

---

## 2. 通用响应格式

除流式接口外，所有接口均遵循统一响应格式：

```json
{
  "success": true,
  "message": "操作结果描述",
  "data": {}
}
```

### 2.1 响应字段

| 字段        | 类型                | 必填 | 说明             |
| --------- | ----------------- | -- | -------------- |
| `success` | `boolean`         | 是  | 请求是否成功         |
| `message` | `string`          | 是  | 请求结果描述         |
| `data`    | `object \| array` | 否  | 响应数据，请求成功时通常存在 |

---

## 3. HTTP 状态码

|   状态码 | 说明                          |
| ----: | --------------------------- |
| `200` | 请求成功                        |
| `400` | 请求参数错误，例如 JSON 格式错误、缺少必填字段等 |
| `404` | 资源不存在，例如指定会话不存在             |
| `500` | 服务器内部错误，例如 AI 模型调用失败        |

---

# 4. API 接口


---

## 4.1 创建新会话

创建一个新的 AI 对话会话，并指定该会话使用的模型。

### 请求

| 项目               | 说明                 |
| ---------------- | ------------------ |
| **URL**          | `/api/session`     |
| **方法**           | `POST`             |
| **Content-Type** | `application/json` |

### 请求体

```json
{
  "model": "deepseek"
}
```

### 请求字段说明

| 字段      | 类型       |  必填 | 说明                       |
| ------- | -------- | :-: | ------------------------ |
| `model` | `string` |  是  | 模型名称，需从 `/api/models` 获取 |

### 响应示例

```json
{
  "success": true,
  "message": "create session success",
  "data": {
    "session_id": "session_1787217829_00000002",
    "model": "deepseek"
  }
}
```

### 响应字段说明

| 字段           | 类型       | 说明        |
| ------------ | -------- | --------- |
| `session_id` | `string` | 新创建的会话 ID |
| `model`      | `string` | 绑定的模型名称   |

---

## 4.2 获取会话列表

获取服务器当前保存的所有会话。

### 请求

| 项目      | 说明              |
| ------- | --------------- |
| **URL** | `/api/sessions` |
| **方法**  | `GET`           |

### 请求示例

```http
GET /api/sessions HTTP/1.1
Host: 服务器IP:8080
```

### 响应示例

```json
{
  "success": true,
  "message": "get session lists success",
  "data": [
    {
      "id": "session_1787217829_00000001",
      "model": "chatgpt",
      "created_at": 1787217829,
      "updated_at": 1787217832,
      "message_count": 2,
      "first_user_message": "你好，我叫小木宁"
    }
  ]
}
```

### 响应字段说明

| 字段                   | 类型       | 说明           |
| -------------------- | -------- | ------------ |
| `id`                 | `string` | 会话唯一标识       |
| `model`              | `string` | 该会话使用的模型名称   |
| `created_at`         | `int64`  | 创建时间戳，单位为秒   |
| `updated_at`         | `int64`  | 最后更新时间戳，单位为秒 |
| `message_count`      | `int`    | 消息总数         |
| `first_user_message` | `string` | 第一条用户消息内容    |

---

## 4.3 获取可用模型列表

获取当前服务器支持的 AI 模型列表。

### 请求

| 项目      | 说明            |
| ------- | ------------- |
| **URL** | `/api/models` |
| **方法**  | `GET`         |

### 请求示例

```http
GET /api/models HTTP/1.1
Host: 服务器IP:8080
```

### 响应示例

```json
{
  "success": true,
  "message": "get model lists success",
  "data": [
    {
      "name": "deepseek-v4-pro",
      "provider": "deepseek"
      "desc": "DeepSeek高性能推理模型"
    },
    {
      "name": "gpt-5.4-mini",
      "provider": "chatgpt"
      "desc": "OpenAI轻量快速"
    },
    {
      "name": "gemini-3.5-flash",
      "provider": "gemini"
      "desc": "Gemini 多模态模型"
    },
    {
      "name": "deepseek-r1:1.5b",
      "provider": "ollama"
      "desc": "本地部署的 DeepSeek R1 1.5B"
    }
  ]
}
```

### 响应字段说明

| 字段     | 类型       | 说明               |
| ------ | -------- | ---------------- |
| `name` | `string` | 模型名称，用于创建会话时指定模型 |
| `provider` | `string` | 模型提供者(系列名)   |
| `desc` | `string` | 模型描述             |

---

## 4.4 删除会话

删除指定的会话。

### 请求

| 项目      | 说明                          |
| ------- | --------------------------- |
| **URL** | `/api/session/{session_id}` |
| **方法**  | `DELETE`                    |

### 路径参数

| 参数           | 类型       |  必填 | 说明        |
| ------------ | -------- | :-: | --------- |
| `session_id` | `string` |  是  | 要删除的会话 ID |

### 请求示例

```http
DELETE /api/session/session_1787217829_00000001 HTTP/1.1
Host: 服务器IP:8080
```

### 响应示例

#### 删除成功

```json
{
  "success": true,
  "message": "delete session success"
}
```

#### 会话不存在

```json
{
  "success": false,
  "message": "delete session failed, session not found"
}
```

---

## 4.5 获取会话历史消息

获取指定会话中的全部历史消息。

### 请求

| 项目      | 说明                                  |
| ------- | ----------------------------------- |
| **URL** | `/api/session/{session_id}/history` |
| **方法**  | `GET`                               |

### 路径参数

| 参数           | 类型       |  必填 | 说明    |
| ------------ | -------- | :-: | ----- |
| `session_id` | `string` |  是  | 会话 ID |

### 请求示例

```http
GET /api/session/session_1787217829_00000001/history HTTP/1.1
Host: 服务器IP:8080
```

### 响应示例

```json
{
  "success": true,
  "message": "get history messages success",
  "data": [
    {
      "id": "msg_1787217829_00000001",
      "role": "user",
      "content": "你好，我叫小木宁",
      "timestamp": 1787217829
    },
    {
      "id": "msg_1787217829_00000002",
      "role": "assistant",
      "content": "你好，小木宁！很高兴认识你。",
      "timestamp": 1787217832
    }
  ]
}
```

### 响应字段说明

| 字段          | 类型       | 说明                        |
| ----------- | -------- | ------------------------- |
| `id`        | `string` | 消息唯一标识                    |
| `role`      | `string` | 消息角色：`user` 或 `assistant` |
| `content`   | `string` | 消息内容                      |
| `timestamp` | `int64`  | 消息时间戳，单位为秒                |

---

## 4.6 发送消息（全量返回）

向指定会话发送一条消息，等待 AI 生成完成后一次性返回完整结果。

### 请求

| 项目               | 说明                 |
| ---------------- | ------------------ |
| **URL**          | `/api/message`     |
| **方法**           | `POST`             |
| **Content-Type** | `application/json` |

### 请求体

```json
{
  "session_id": "session_1787217829_00000001",
  "message": "我叫什么名字？"
}
```

### 请求字段说明

| 字段           | 类型       |  必填 | 说明        |
| ------------ | -------- | :-: | --------- |
| `session_id` | `string` |  是  | 会话 ID     |
| `message`    | `string` |  是  | 用户发送的消息内容 |

### 响应示例

```json
{
  "success": true,
  "message": "send message success",
  "data": {
    "session_id": "session_1787217829_00000001",
    "response": "你叫小木宁呀！"
  }
}
```

### 响应字段说明

| 字段           | 类型       | 说明         |
| ------------ | -------- | ---------- |
| `session_id` | `string` | 当前会话 ID    |
| `response`   | `string` | AI 的完整回复内容 |

---

## 4.7 发送消息（流式返回）

向指定会话发送一条消息，通过 **Server-Sent Events（SSE）** 持续返回 AI 生成的文本片段。

与 `/api/message` 不同，该接口无需等待 AI 完成全部内容后再返回，而是随着模型生成过程逐步返回数据。

### 请求

| 项目                  | 说明                   |
| ------------------- | -------------------- |
| **URL**             | `/api/message/async` |
| **方法**              | `POST`               |
| **Content-Type**    | `application/json`   |
| **响应 Content-Type** | `text/event-stream`  |

### 请求体

```json
{
  "session_id": "session_1787217829_00000001",
  "message": "讲个笑话"
}
```

### 请求字段说明

| 字段           | 类型       |  必填 | 说明        |
| ------------ | -------- | :-: | --------- |
| `session_id` | `string` |  是  | 会话 ID     |
| `message`    | `string` |  是  | 用户发送的消息内容 |

### 响应格式

服务器通过 SSE 持续发送数据，每条消息以两个换行符 `\n\n` 结尾。

```text
data: "文本片段1"

data: "文本片段2"

data: "文本片段3"

data: [DONE]
```

### SSE 数据说明

| 数据             | 说明           |
| -------------- | ------------ |
| `data: "文本"`   | AI 回复的一个文本片段 |
| `data: [DONE]` | 流式响应结束标记     |

### 流式响应示例

```text
data: "从前有一只小猫，"

data: "它特别喜欢吃鱼。"

data: "有一天，它发现了一条大鱼！"

data: [DONE]
```

> **注意：**
>
> 1. 每个 `data:` 后面跟一个空格。
> 2. 每条 SSE 消息以两个换行符 `\n\n` 结尾。
> 3. 客户端应持续读取 SSE 数据，直到收到 `data: [DONE]`。
> 4. 该接口响应类型为 `text/event-stream`，不能按照普通 JSON 响应进行解析。

---

# 5. 接口汇总

接口顺序与 `ChatServer` 类中的请求处理函数声明顺序保持一致。

| 序号 | 接口                                  | 方法       | 功能        | 返回类型 |
| -: | ----------------------------------- | -------- | --------- | ---- |
|  1 | `/api/session`                      | `POST`   | 创建新会话     | JSON |
|  2 | `/api/sessions`                     | `GET`    | 获取会话列表    | JSON |
|  3 | `/api/models`                       | `GET`    | 获取可用模型列表  | JSON |
|  4 | `/api/session/{session_id}`         | `DELETE` | 删除会话      | JSON |
|  5 | `/api/session/{session_id}/history` | `GET`    | 获取历史消息    | JSON |
|  6 | `/api/message`                      | `POST`   | 发送消息并完整返回 | JSON |
|  7 | `/api/message/async`                | `POST`   | 发送消息并流式返回 | SSE  |

---

# 6. API 与 ChatServer 对应关系

API 接口与 `ChatServer` 类中的请求处理函数对应关系如下：

| ChatServer 成员函数                         | HTTP 方法  | HTTP 路径                             | 功能     |
| --------------------------------------- | -------- | ----------------------------------- | ------ |
| `handle_create_session_request()`       | `POST`   | `/api/session`                      | 创建新会话  |
| `handle_get_session_lists_request()`    | `GET`    | `/api/sessions`                     | 获取会话列表 |
| `handle_get_model_lists_request()`      | `GET`    | `/api/models`                       | 获取模型列表 |
| `handle_delete_session_request()`       | `DELETE` | `/api/session/{session_id}`         | 删除会话   |
| `handle_get_history_messages_request()` | `GET`    | `/api/session/{session_id}/history` | 获取历史消息 |
| `handle_send_message_request()`         | `POST`   | `/api/message`                      | 全量发送消息 |
| `handle_send_message_stream_request()`  | `POST`   | `/api/message/async`                | 流式发送消息 |

---

# 7. 推荐调用流程

一个完整的 AI 聊天流程如下：

```text
┌──────────────────────────────┐
│  1. 获取可用模型              │
│     GET /api/models          │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│  2. 创建会话                  │
│     POST /api/session        │
└──────────────┬───────────────┘
               │
               │ session_id
               ▼
┌──────────────────────────────┐
│  3. 发送消息                  │
│     POST /api/message        │
│              或              │
│     POST /api/message/async  │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│  4. 获取历史消息              │
│     GET /api/session/{id}    │
│              /history        │
└──────────────────────────────┘
```

如需查看当前所有会话：

```http
GET /api/sessions
```

如需删除指定会话：

```http
DELETE /api/session/{session_id}
```

---

# 8. 接口设计说明

本服务采用 **HTTP REST API + SSE 流式通信** 的方式提供 AI 聊天服务。

接口按照功能划分为以下几个部分：

### 8.1 会话管理

负责 AI 对话会话的创建、查询和删除。

* `POST /api/session`
* `GET /api/sessions`
* `DELETE /api/session/{session_id}`

### 8.2 模型管理

负责获取当前服务器支持的 AI 模型。

* `GET /api/models`

### 8.3 历史消息

负责获取指定会话中的历史聊天记录。

* `GET /api/session/{session_id}/history`

### 8.4 消息发送

提供两种消息发送方式：

**全量返回：**

```text
POST /api/message
```

等待 AI 完成生成后，一次性返回完整回复。

**流式返回：**

```text
POST /api/message/async
```

使用 SSE 持续返回 AI 生成的文本片段。

---

# 9. ChatServer 接口声明对应

当前 `ChatServer` 类中的 HTTP 请求处理函数按照以下顺序组织：

```cpp
// 处理创建会话请求
void handle_create_session_request(
    const httplib::Request& request,
    httplib::Response& response
);

// 处理获取会话列表请求
void handle_get_session_lists_request(
    const httplib::Request& request,
    httplib::Response& response
);

// 处理获取模型列表请求
void handle_get_model_lists_request(
    const httplib::Request& request,
    httplib::Response& response
);

// 处理删除会话请求
void handle_delete_session_request(
    const httplib::Request& request,
    httplib::Response& response
);

// 处理获取历史消息请求
void handle_get_history_messages_request(
    const httplib::Request& request,
    httplib::Response& response
);

// 处理发送消息请求 - 全量返回
void handle_send_message_request(
    const httplib::Request& request,
    httplib::Response& response
);

// 处理发送消息请求 - 增量返回
void handle_send_message_stream_request(
    const httplib::Request& request,
    httplib::Response& response
);
```

以上函数分别对应第 **4.1 ~ 4.7** 节中的 API 接口。

---

## 10. 版本信息

| 项目              | 内容                      |
| --------------- | ----------------------- |
| **API Version** | `v1`                    |
| **文档版本**        | `1.0.0`                 |
| **协议**          | HTTP/1.1                |
| **数据格式**        | JSON                    |
| **流式协议**        | Server-Sent Events（SSE） |
| **字符编码**        | UTF-8                   |
