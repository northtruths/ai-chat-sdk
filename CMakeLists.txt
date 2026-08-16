cmake_minimum_required(VERSION 3.10)
project(AIChatSDK)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 OpenSSL
find_package(OpenSSL REQUIRED)

# 查找 GTest（如果不需要可以注释掉）
find_package(GTest REQUIRED)

# ★ 收集 SDK 源文件
file(GLOB_RECURSE SDK_SRCS "sdk/src/*.cpp")

# 只保留需要的
list(FILTER SDK_SRCS INCLUDE REGEX "DeepSeek|Ollama|Gemini|utils|Common|DataManager|LLMManager|Session|BaseConfig|ChatSDK")

# 测试文件
file(GLOB_RECURSE TEST_SRCS "tests/gemini*.cpp")

# ============================================================
#  目标1：编译静态库（给其他人用）
# ============================================================
add_library(ai_chat_sdk STATIC ${SDK_SRCS})

target_include_directories(ai_chat_sdk PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/include/utils
)

target_compile_definitions(ai_chat_sdk PRIVATE
    CPPHTTPLIB_OPENSSL_SUPPORT
)

# ★ 库的链接（之前漏了！）
target_link_libraries(ai_chat_sdk
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
    jsoncpp
    z
    dl
    sqlite3
)

# ============================================================
#  目标2：编译测试程序（自己调试用）
# ============================================================
add_executable(test_chat ${SDK_SRCS} ${TEST_SRCS})

target_include_directories(test_chat PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/include/utils
)

target_compile_definitions(test_chat PRIVATE
    CPPHTTPLIB_OPENSSL_SUPPORT
)

target_link_libraries(test_chat
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
    jsoncpp
    z
    dl
    sqlite3
    gtest
    gtest_main
)

# ============================================================
#  目标3：安装库到系统
# ============================================================
install(TARGETS ai_chat_sdk
    ARCHIVE DESTINATION lib
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/sdk/include/
    DESTINATION include/ai_chat_sdk
    FILES_MATCHING PATTERN "*.hpp"
)

# ============================================================
#  辅助目标
# ============================================================
add_custom_target(cleanlog
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/logs
    COMMENT "清理日志文件夹"
)

add_custom_target(cleanall
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_BINARY_DIR}/CMakeCache.txt
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_BINARY_DIR}/Makefile
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/logs
    COMMENT "清理所有编译产物和日志"
)