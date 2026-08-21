#include <gtest/gtest.h>
#include <fstream>
#include <jsoncpp/json/json.h>

#include "DeepSeekProvider.hpp"
#include "DeepSeekConfig.hpp"
#include "utils/logger.hpp"

using namespace ai_chat_sdk;

// ============================================================
//  测试工具：加载配置
// ============================================================
std::string get_api_key()
{
    std::ifstream file("../config.json");
    if (!file.is_open()) {
        return "";
    }
    Json::Value root;
    Json::Reader reader;
    reader.parse(file, root);
    return root.get("api_key", "").asString();
}

// ============================================================
//  测试：流式发送
// ============================================================
TEST(DeepSeekStreamTest, SendMessageStream)
{
    std::string api_key = get_api_key();
    if (api_key.empty()) {
        GTEST_SKIP() << "跳过：未找到 config.json 或 api_key 为空";
    }

    DeepSeekConfig config;
    config.set_api_key(api_key);
    config.set_url("https://api.deepseek.com", "/chat/completions");

    DeepSeekProvider provider(&config);

    std::string full_reply;
    auto callback = [&](const std::string& chunk, bool last) {
        std::cout << chunk << std::flush;
        full_reply += chunk;
        if (last) {
            std::cout << std::endl;
        }
    };

    std::string reply = provider.send_message_stream("你好，请用一句话介绍自己", callback);

    EXPECT_FALSE(reply.empty());
    EXPECT_EQ(reply, full_reply);
}

// ============================================================
//  测试：流式发送（空消息）
// ============================================================
TEST(DeepSeekStreamTest, SendEmptyMessage)
{
    std::string api_key = get_api_key();
    if (api_key.empty()) {
        GTEST_SKIP() << "跳过：未找到 config.json 或 api_key 为空";
    }

    DeepSeekConfig config;
    config.set_api_key(api_key);
    config.set_url("https://api.deepseek.com", "/chat/completions");

    DeepSeekProvider provider(&config);

    std::string full_reply;
    auto callback = [&](const std::string& chunk, bool last) {
        full_reply += chunk;
    };

    std::string reply = provider.send_message_stream("", callback);

    EXPECT_TRUE(reply.empty());
    EXPECT_TRUE(full_reply.empty());
}

// ============================================================
//  主函数
// ============================================================
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    // 初始化日志（让测试时能看到日志输出）
    mylog::Logger& logger = mylog::Logger::instance();
    logger.set_level(mylog::Level::TRACE);
    logger.set_formatter(mylog::make_default_formatter());
    logger.set_transmitter(mylog::make_async_transmitter(4 * 1024 * 1024, 1000));
    logger.add_sink(mylog::make_file_sink("../logs", "stream_test.log"));

    return RUN_ALL_TESTS();
}