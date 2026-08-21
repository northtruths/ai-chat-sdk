cmake_minimum_required(VERSION 3.16)
project(AIChatServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 可执行文件输出到项目根目录的 bin/ 文件夹
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../bin)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(gflags CONFIG REQUIRED)
find_package(PkgConfig QUIET)

find_path(AI_CHAT_SDK_INCLUDE_DIR
    NAMES ChatSDK.hpp
    PATHS
        /usr/local/include/ai_chat_sdk
        /usr/include/ai_chat_sdk
    REQUIRED
)

find_library(AI_CHAT_SDK_LIBRARY
    NAMES ai_chat_sdk
    PATHS
        /usr/local/lib
        /usr/lib
    REQUIRED
)

find_path(HTTPLIB_INCLUDE_DIR
    NAMES httplib.h
    PATHS
        /usr/local/include
        /usr/include
    REQUIRED
)

find_path(JSONCPP_INCLUDE_DIR
    NAMES jsoncpp/json/json.h
    PATHS
        /usr/local/include
        /usr/include
    REQUIRED
)

find_library(JSONCPP_LIBRARY
    NAMES jsoncpp
    PATHS
        /usr/local/lib
        /usr/lib
    REQUIRED
)

add_executable(AIChatServer
    main.cpp
    ChatServer.cpp
)

target_include_directories(AIChatServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${AI_CHAT_SDK_INCLUDE_DIR}
    ${HTTPLIB_INCLUDE_DIR}
    ${JSONCPP_INCLUDE_DIR}
)

target_compile_definitions(AIChatServer PRIVATE
    CPPHTTPLIB_OPENSSL_SUPPORT
)

target_link_libraries(AIChatServer PRIVATE
    ${AI_CHAT_SDK_LIBRARY}
    gflags
    ${JSONCPP_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    sqlite3
    dl
    z
)

# ============================================================
#  安装：将可执行文件安装到 bin/（这里已经输出到 bin/ 了）
# ============================================================
install(TARGETS AIChatServer RUNTIME DESTINATION bin)

# ============================================================
#  辅助目标
# ============================================================

# 清理日志文件
add_custom_target(clean_logs
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/*.log
    COMMENT "清理日志文件"
)

# 清理数据库文件
add_custom_target(clean_db
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/chat.db
    COMMENT "清理数据库文件"
)

# 清理配置文件（慎用）
add_custom_target(clean_config
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/config.json
    COMMENT "清理配置文件（慎用）"
)

# 清理所有运行时数据（日志 + 数据库）
add_custom_target(clean_data
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/*.log
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/chat.db
    COMMENT "清理所有运行时数据（日志 + 数据库）"
)

# 清理编译产物（不删除 bin/ 里的可执行文件）
add_custom_target(clean_build
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "清理编译产物（build/ 目录）"
)

# 清理所有（编译产物 + 运行时数据 + 可执行文件）
add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/*.log
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/chat.db
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/../bin
    COMMENT "清理所有：编译产物 + 运行时数据 + 可执行文件"
)