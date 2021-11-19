# CMakeList.txt: uartframeparser 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required(VERSION 3.8)

project("uartframeparser")

# 将源代码添加到此项目的可执行文件。
add_library(uartframeparser STATIC "config_parse.c" "frame_parse.c" "uartframeparser.h" "buffer.c" "expression.c")

set(_IMPORT_PREFIX "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
find_package(cJSON REQUIRED)
target_link_libraries(uartframeparser PRIVATE cjson)

find_package(Lua REQUIRED)
target_include_directories(uartframeparser PRIVATE ${LUA_INCLUDE_DIR})
target_link_libraries(uartframeparser PRIVATE ${LUA_LIBRARIES})

# 添加测试
find_package(GTest)

if (${GTEST_FOUND})

    add_executable(basic_function_test "test.cpp")

    target_include_directories(basic_function_test PRIVATE ${GTEST_INCLUDE_DIR})
    target_link_libraries(basic_function_test PRIVATE ${GTEST_LIBRARIES} uartframeparser)

    enable_testing()
    add_test(NAME basic_function_test COMMAND basic_function_test)

endif ()