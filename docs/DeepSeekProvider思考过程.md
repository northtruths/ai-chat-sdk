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

# 7.27
全量发送有点BUG，httplib 默认只支持 HTTP，但deepseek只接收HTTPS所以要启用 OpenSSL 支持
然后 httplib 版本和 OpenSSL 版本不兼容，httplib 太高，降级就行或者 OpenSSL 升级
咨询AI升级OpenSSL可能影响系统，然后降级了 httplib 代码有些函数又不支持须要更改
最后安装了 OpenSSL 3.0 通过Makefile增加查找路径

# 7.28
之前两种方式都试过，但发送还是无法成功，连接不上deepseek
然后换了种编译方式，使用 CMake 进行自动生成 makefile 然后就行了，然后修复了点代码 bug ，所以之前应该就是编译问题，没有正确链接上 SSL 或某个文件

# 7.29
流式信息发送 
和全量一样，不过要给httplib一个回调函数，因为每次接受一点数据，httplib就会调用这个函数，然后它发送的每一段信息都由\n\n结尾？我们就是拼接下来即可
流式发送数据格式（SEE格式）
data: {内容}\n\n
data: {"id":"xxx","choices":[{"delta":{"role":"assistant","content":""},"finish_reason":null}]}
data: {"id":"xxx","choices":[{"index":0,"delta":{"content":"你"},"finish_reason":null}]}
data: {"id":"xxx","choices":[{"index":0,"delta":{"content":"好"},"finish_reason":null}]}
data: {"id":"xxx","choices":[{"index":0,"delta":{"content":"！"},"finish_reason":null}]}
data: [DONE]

流式响应结构
{
  "choices": [{
    "delta": {           // ← 是 delta，不是 message
      "content": "你"
    }
  }]
}

实现过程
1. 检查模型和key有效
2. 构建json信息（开启 deepseek 的流式响应字段）
3. json序列化
4. 创建http客户端(第三方库cpp-httplib)
5. 等待并获取返回信息
6. 解析并处理返回信息

两个要点：
发送请求从函数传参改为构建请求对象更清晰明了，主要是多了两个重要参数
1. response_handler 响应处理器，在启动流式响应后，服务器会将整个信息拆分，进行流式发送，但 http 报头是一定完整发送的，response_handler 就用来处理响应头，进行快速诊断错误，避免多余处理浪费资源
ResponseHandler response_handler;//声明
std::function<void(const Response&)>//实际定义类型，是个函数包装器
2. content_receiver 内容接收器，在启动流式响应后，服务器会将整个信息拆分，进行流式发送，除了 http 报头，其他的信息都可能是流式、不完整的，同样它是一个函数包装器，开启后给予它回调函数，它会再每次收到内容体时调用回调，以此来处理接收的部分信息体
ContentReceiverWithProgress content_receiver；//声明
function<bool(const char* data, size_t len, uint64_t offset, uint64_ttotal)>//实际定义类型
data是指向当前接收到的数据块的指针，len是当前数据块长度
offset是当前数据块在请求体中的偏移量，total是请求体的总长度
返回值为布尔，true表示继续接收(正常连接着)，false表示停止接收(中途有错误，停止发送)