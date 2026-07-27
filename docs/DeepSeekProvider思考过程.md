在之前统一将所有参数用Config类封装了起来
抽象类加了参数相关操作函数
现在继承基类直接完善deepseek类即可
初始化模型：查找提供的config是否包含必要参数，没有则初始化失败
发现信息类和Config类功能完全重合且统一用Json更好，所以删除冗余并合并
因为目前大部分大语言模型

发送信息
1. 检查模型和key有效
2. 构建json信息
3. json序列化
4. http客户端发送序列化信息(第三方库cpp-httplib)
5. 等待并获取返回信息
6. 解析并处理返回信息

deepseek完整请求信息格式
curl -L -X POST 'https://api.deepseek.com/chat/completions' \
-H 'Content-Type: application/json' \
-H 'Accept: application/json' \
-H 'Authorization: Bearer <TOKEN>' \
--data-raw '{
  "messages": [
    {
      "content": "You are a helpful assistant",
      "role": "system"
    },
    {
      "content": "Hi",
      "role": "user"
    }
  ],
  "model": "deepseek-v4-pro",
  "thinking": {
    "type": "enabled"
  },
  "reasoning_effort": "high",
  "max_tokens": 4096,
  "response_format": {
    "type": "text"
  },
  "stop": null,
  "stream": false,
  "stream_options": null,
  "temperature": 1,
  "top_p": 1,
  "tools": null,
  "tool_choice": "none",
  "logprobs": false,
  "top_logprobs": null
}'

# 7.26
结构完全进行了重构，现在是Config类和Provider类，
一个Config类组织管理所有参数，进行统一接口
一个Provider类组织管理Config，负责API调用，和具体的 HTTP 请求、JSON 拼装、流式解析等

deepseek的完整返回json
{
  "id": "chatcmpl-xxx",
  "object": "chat.completion",
  "created": 1700000000,
  "model": "deepseek-chat",
  "choices": [
    {
      "index": 0,
      "message": {
        "role": "assistant",
        "content": "你好！我是 DeepSeek，有什么可以帮你的？"
      },
      "finish_reason": "stop"
    }
  ],
  "usage": {
    "prompt_tokens": 10,
    "completion_tokens": 15,
    "total_tokens": 25
  }
}