cmake_minimum_required(VERSION 3.8)

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
set(ENABLE_CJSON_TEST OFF CACHE INTERNAL "" FORCE)
set(CJSON_BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
set(ENABLE_PUBLIC_SYMBOLS OFF CACHE INTERNAL "" FORCE)
set(BUILD_GMOCK OFF CACHE INTERNAL "" FORCE)
set(INSTALL_GTEST OFF CACHE INTERNAL "" FORCE)
set(gtest_force_shared_crt ON CACHE INTERNAL "" FORCE)
set(gtest_disable_pthreads ON CACHE INTERNAL "" FORCE)

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

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        703bd9caab50b139428cea1aaff9974ebee5742e # release-1.10.0
)

FetchContent_MakeAvailable(cjson googletest)
FetchContent_Populate(lua)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/lua/CMakeLists.txt DESTINATION ${lua_SOURCE_DIR})
add_subdirectory(${lua_SOURCE_DIR} ${lua_BINARY_DIR})

set (LIBUARTFRAMEPARSER_NAME "uartframeparser")
set (UARTFRAMEPARSER_TEST_NAME ${LIBUARTFRAMEPARSER_NAME}test)

include_directories("include" "src")

add_library(${LIBUARTFRAMEPARSER_NAME} "src/config_parse.c" "src/frame_parse.c" "include/uartframeparser.h" "src/buffer.c" "src/expression.c" "src/frame_read.c" "src/frame_decode.c" "test/test.cpp")

if (MSVC)
    set(gtest_for)
    target_compile_definitions(${LIBUARTFRAMEPARSER_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

target_link_libraries(${LIBUARTFRAMEPARSER_NAME}
    LINK_PRIVATE debug gtestd
    LINK_PRIVATE debug gtest_maind
    LINK_PRIVATE optimized gtest
    LINK_PRIVATE optimized gtest_main
    LINK_PUBLIC cjson
    LINK_PRIVATE lua
)
target_include_directories(${LIBUARTFRAMEPARSER_NAME} PUBLIC ${cjson_SOURCE_DIR} PRIVATE ${lua_SOURCE_DIR} PRIVATE ${googletest_SOURCE_DIR}/googletest/include)

set_property(TARGET ${LIBUARTFRAMEPARSER_NAME} PROPERTY CXX_STANDARD 11)

add_executable(${UARTFRAMEPARSER_TEST_NAME} "test/main.c")
target_link_libraries(${UARTFRAMEPARSER_TEST_NAME} LINK_PRIVATE ${LIBUARTFRAMEPARSER_NAME})

enable_testing()
add_test(NAME ${UARTFRAMEPARSER_TEST_NAME} COMMAND ${UARTFRAMEPARSER_TEST_NAME})
