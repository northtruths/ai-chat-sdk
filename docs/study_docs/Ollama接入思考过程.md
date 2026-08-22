ollama作为本地模型，可以自定义各种模型框架，本次使用最基础的deepseek-r1:1.5b作为测试，因此几乎所有配置、结构、框架都和deepseek一模一样
ollama 不需要api_key，url为 "http://localhost:11434/api/chat"
因为是本地连接，所以网络层协议几乎不会错，但为了严谨并且简单，直接复用以前代码

经过测试ollama的非流式response的json串如下：
{
    "model": "deepseek-r1:1.5b",
    "created_at": "2026-08-04T00:54:35.449936269Z",
    "message": {
        "role": "assistant",
        "content": "\u003cthink\u003e\n\n\u003c/think\u003e\n\n你好！很高兴见到你，有什么我可以帮忙的吗？"
    },
    "done_reason": "stop",
    "done": true,
    "total_duration": 8161859633,
    "load_duration": 5549589537,
    "prompt_eval_count": 4,
    "prompt_eval_duration": 431000000,
    "eval_count": 17,
    "eval_duration": 2101000000
}

流式response格式如下：
{"model":"deepseek-r1:1.5b","created_at":"2026-08-04T00:59:06.109901432Z","message":{"role":"assistant","content":"\u003cthink\u003e"},"done":false}
{"model":"deepseek-r1:1.5b","created_at":"2026-08-04T00:59:06.247729957Z","message":{"role":"assistant","content":"\n\n"},"done":false}
{"model":"deepseek-r1:1.5b","created_at":"2026-08-04T00:59:06.368005586Z","message":{"role":"assistant","content":"\u003c/think\u003e"},"done":false}
...
{"model":"deepseek-r1:1.5b","created_at":"2026-08-04T00:59:09.796136521Z","message":{"role":"assistant","content":"😊"},"done":false}
{"model":"deepseek-r1:1.5b","created_at":"2026-08-04T00:59:09.921367714Z","message":{"role":"assistant","content":""},"done_reason":"stop","done":true,"total_duration":4100611371,"load_duration":126949998,"prompt_eval_count":4,"prompt_eval_duration":144000000,"eval_count":31,"eval_duration":3820000000}


错误测试，错误返回信息只有 error 一个字段
curl http://localhost:11434/api/chat -d '{
  "model": "未知模型",
  "messages": [{"role": "user", "content": "你好"}],
  "stream": false,
  "test": 0
}'

{"error":"model is required"}