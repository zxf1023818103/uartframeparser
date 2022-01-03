cmake_minimum_required(VERSION 3.8)

set(BUILD_SHARED_LIBS no)
set(ENABLE_CJSON_TEST off)
set(ENABLE_TARGET_EXPORT off)

project("uartframeparser")

include(FetchContent)

FetchContent_Declare(
    cjson
    GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
    GIT_TAG d348621ca93571343a56862df7de4ff3bc9b5667 # v1.7.15
)

FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG eadd8c7178c79c814ecca9652973a9b9dd4cc71b # v5.4.3
)

FetchContent_MakeAvailable(cjson)
FetchContent_Populate(lua)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/lua/CMakeLists.txt DESTINATION ${lua_SOURCE_DIR})
add_subdirectory(${lua_SOURCE_DIR} ${lua_BINARY_DIR})

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set (LIBUARTFRAMEPARSER_NAME "uartframeparserd")
    set (UARTFRAMEPARSER_TEST_NAME "uartframeparsertestd")
else()
    set (LIBUARTFRAMEPARSER_NAME "uartframeparser")
    set (UARTFRAMEPARSER_TEST_NAME "uartframeparsertest")
endif()

include_directories("include" "src")

# 将源代码添加到此项目的可执行文件。
add_library(${LIBUARTFRAMEPARSER_NAME} "src/config_parse.c" "src/frame_parse.c" "include/uartframeparser.h" "src/buffer.c" "src/expression.c" "src/frame_read.c" "src/frame_decode.c")

target_link_libraries(${LIBUARTFRAMEPARSER_NAME} PRIVATE cjson lua)
target_include_directories(${LIBUARTFRAMEPARSER_NAME} PRIVATE ${cjson_SOURCE_DIR} ${lua_SOURCE_DIR})
