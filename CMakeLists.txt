# CMakeList.txt: uartframeparser 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

project ("uartframeparser")

# 将源代码添加到此项目的可执行文件。
add_executable (uartframeparser "main.c" "uartframeparser.h")

set(_IMPORT_PREFIX "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
find_package(cJSON REQUIRED)
target_link_libraries(uartframeparser PRIVATE cjson)

find_package(Lua REQUIRED)
target_include_directories(uartframeparser PRIVATE ${LUA_INCLUDE_DIR})
target_link_libraries(uartframeparser PRIVATE ${LUA_LIBRARIES})

# TODO: 如有需要，请添加测试并安装目标。
