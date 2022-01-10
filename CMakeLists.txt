cmake_minimum_required(VERSION 3.8)

set(ENABLE_LOCALES OFF CACHE INTERNAL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
set(ENABLE_CJSON_TEST OFF CACHE INTERNAL "" FORCE)
set(CJSON_BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
set(CJSON_OVERRIDE_BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
set(ENABLE_PUBLIC_SYMBOLS OFF CACHE INTERNAL "" FORCE)
set(ENABLE_HIDDEN_SYMBOLS ON CACHE INTERNAL "" FORCE)
set(BUILD_GMOCK OFF CACHE INTERNAL "" FORCE)
set(INSTALL_GTEST OFF CACHE INTERNAL "" FORCE)
set(ENABLE_TARGET_EXPORT OFF CACHE INTERNAL "" FORCE)
set(gtest_force_shared_crt ON CACHE INTERNAL "" FORCE)
set(gtest_disable_pthreads ON CACHE INTERNAL "" FORCE)

project("uartframeparser")

option(ENABLE_TESTING OFF "Enable Testing")

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

if (ZEPHYR_BASE)
    link_libraries(zephyr_interface)
endif()

FetchContent_MakeAvailable(cjson)
FetchContent_Populate(lua)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/lua/CMakeLists.txt DESTINATION ${lua_SOURCE_DIR})
add_subdirectory(${lua_SOURCE_DIR} ${lua_BINARY_DIR})

set (LIBUARTFRAMEPARSER_NAME "uartframeparser")
set (UARTFRAMEPARSER_TEST_NAME ${LIBUARTFRAMEPARSER_NAME}test)

include_directories("include" "src" ${cjson_SOURCE_DIR} ${lua_SOURCE_DIR})

add_library(${LIBUARTFRAMEPARSER_NAME}core OBJECT "src/config_parse.c" "src/frame_parse.c" "include/uartframeparser.h" "src/buffer.c" "src/expression.c" "src/frame_read.c" "src/frame_decode.c")
add_library(${LIBUARTFRAMEPARSER_NAME} STATIC $<TARGET_OBJECTS:${LIBUARTFRAMEPARSER_NAME}core> $<TARGET_OBJECTS:cjson> $<TARGET_OBJECTS:lua>)

if (MSVC)
    set(ENABLE_TESTING ON)
    target_compile_definitions(${LIBUARTFRAMEPARSER_NAME}core PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()

if (ENABLE_TESTING)
    FetchContent_MakeAvailable(googletest)
    include_directories(${googletest_SOURCE_DIR}/googletest/include)
    add_executable(${UARTFRAMEPARSER_TEST_NAME} "test/test.cpp" $<TARGET_OBJECTS:${LIBUARTFRAMEPARSER_NAME}core> $<TARGET_OBJECTS:lua> $<TARGET_OBJECTS:gtest>)
    enable_testing()
    add_test(NAME ${UARTFRAMEPARSER_TEST_NAME} COMMAND ${UARTFRAMEPARSER_TEST_NAME})
endif()
