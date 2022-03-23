cmake_minimum_required(VERSION 3.8)

if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    include(FetchContent)
    FetchContent_Declare(vcpkg
        vcpkg
        GIT_REPOSITORY "https://github.com/microsoft/vcpkg.git"
        GIT_TAG origin/master)
    FetchContent_MakeAvailable(vcpkg)
    set(CMAKE_TOOLCHAIN_FILE ${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake)
endif()

project("uartframeparser")

set(CMAKE_WIN32_EXECUTABLE TRUE)

# 禁用 MSVC 的使用不安全 C 语言标准库函数时弹出的警告
if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set (LIBUARTFRAMEPARSER_NAME "uartframeparserd")
    set (UARTFRAMEPARSER_GUI_NAME "quartframeparserd")
    set (UARTFRAMEPARSER_TEST_NAME "uartframeparsertestd")
else()
    set (LIBUARTFRAMEPARSER_NAME "uartframeparser")
    set (UARTFRAMEPARSER_GUI_NAME "quartframeparser")
    set (UARTFRAMEPARSER_TEST_NAME "uartframeparsertest")
endif()

include_directories("include" "src")

# 将源代码添加到此项目的可执行文件。
add_library(${LIBUARTFRAMEPARSER_NAME} "lib/config_parse.c" "lib/frame_parse.c" "include/uartframeparser.h" "lib/buffer.c" "lib/expression.c" "lib/frame_read.c" "lib/frame_decode.c")

add_dependencies(${LIBUARTFRAMEPARSER_NAME} cjson ${Lua_LIBRARIES})

set(_IMPORT_PREFIX "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
find_package(cJSON REQUIRED)
target_link_libraries(${LIBUARTFRAMEPARSER_NAME} PRIVATE cjson)

find_package(Lua REQUIRED)
target_link_libraries(${LIBUARTFRAMEPARSER_NAME} PRIVATE ${LUA_LIBRARIES})
target_include_directories(${LIBUARTFRAMEPARSER_NAME} PRIVATE ${LUA_INCLUDE_DIR})

find_package(Git REQUIRED)

execute_process(COMMAND ${GIT_EXECUTABLE} log --pretty=format:'%h' -n 1
                OUTPUT_VARIABLE GIT_REV
                ERROR_QUIET)

if ("${GIT_REV}" STREQUAL "")
    set(GIT_REV "N/A")
    set(GIT_DIFF "")
    set(GIT_TAG "N/A")
    set(GIT_BRANCH "N/A")
else()
    execute_process(
        COMMAND ${GIT_EXECUTABLE} diff --quiet --exit-code
        OUTPUT_VARIABLE GIT_DIFF
        RESULT_VARIABLE GIT_DIFF_RETURN)
    if (${GIT_DIFF_RETURN} EQUAL 0)
        set(GIT_DIFF "+")
    endif()

    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --exact-match --tags
        OUTPUT_VARIABLE GIT_TAG ERROR_QUIET)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        OUTPUT_VARIABLE GIT_BRANCH)

    string(STRIP "${GIT_REV}" GIT_REV)
    string(SUBSTRING "${GIT_REV}" 1 7 GIT_REV)
    string(STRIP "${GIT_DIFF}" GIT_DIFF)
    string(STRIP "${GIT_TAG}" GIT_TAG)
    string(STRIP "${GIT_BRANCH}" GIT_BRANCH)
endif()

add_compile_definitions(GIT_REV="${GIT_REV}")
add_compile_definitions(GIT_BRANCH="${GIT_BRANCH}")
if (NOT "${GIT_TAG}" STREQUAL "")
    add_compile_definitions(GIT_TAG="${GIT_TAG}")
endif()

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Gui Widgets LinguistTools SerialPort)
if (QT_FOUND)

    set(CMAKE_INCLUDE_CURRENT_DIR ON)

    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Gui Widgets LinguistTools SerialPort REQUIRED)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Widgets LinguistTools SerialPort REQUIRED)

    set(TS_FILES src/uartframeparser_zh_CN.ts)

    set(PROJECT_SOURCES
            src/main.cpp
            src/mainwindow.cpp
            src/mainwindow.h
            src/mainwindow.ui
            src/settingsdialog.cpp
            src/settingsdialog.h
            src/settingsdialog.ui
            src/framedefinitiondialog.cpp
            src/framedefinitiondialog.h
            src/framedefinitiondialog.ui
            src/fielddefinitiondialog.cpp
            src/fielddefinitiondialog.h
            src/fielddefinitiondialog.ui
            src/bitfielddefinitiondialog.cpp
            src/bitfielddefinitiondialog.h
            src/bitfielddefinitiondialog.ui
            src/subframedialog.cpp
            src/subframedialog.h
            src/subframedialog.ui
            src/savechangesdialog.cpp
            src/savechangesdialog.h
            src/savechangesdialog.ui
            src/aboutdialog.cpp
            src/aboutdialog.h
            src/aboutdialog.ui
            src/logtextedit.cpp
            src/logtextedit.h
            src/uartframeparserwrapper.cpp
            src/uartframeparserwrapper.h
            ${TS_FILES}
    )

    if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
        qt_add_executable(${UARTFRAMEPARSER_GUI_NAME}
            MANUAL_FINALIZATION
            ${PROJECT_SOURCES}
        )
        qt_create_translation(MESSAGES ${PROJECT_SOURCES} ${TS_FILES})
        qt_add_translation(QM_FILES ${TS_FILES})
        add_custom_target(translations DEPENDS ${QM_FILES})
        add_dependencies(${UARTFRAMEPARSER_GUI_NAME} translations)
    # Define target properties for Android with Qt 6 as:
    #    set_property(TARGET uartframeparser APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
    #                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
    # For more information, see https://doc.qt.io/qt-6/qt-add-executable.html·#target-creation
    else()
        if(ANDROID)
            add_library(${UARTFRAMEPARSER_GUI_NAME} SHARED
                ${PROJECT_SOURCES}
            )
    # Define properties for Android with Qt 5 after find_package() calls as:
    #    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
        else()
            add_executable(${UARTFRAMEPARSER_GUI_NAME}
                ${PROJECT_SOURCES}
            )
        endif()

        qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
    endif()

    target_link_libraries(${UARTFRAMEPARSER_GUI_NAME} PRIVATE Qt::Core Qt6::Core Qt::CorePrivate Qt6::CorePrivate)
    target_link_libraries(${UARTFRAMEPARSER_GUI_NAME} PRIVATE Qt::EntryPointPrivate Qt6::EntryPointPrivate Qt6::EntryPointImplementation)
    target_link_libraries(${UARTFRAMEPARSER_GUI_NAME} PRIVATE Qt::Gui Qt6::Gui Qt::GuiPrivate Qt6::GuiPrivate)
    target_link_libraries(${UARTFRAMEPARSER_GUI_NAME} PRIVATE Qt::Widgets Qt6::Widgets Qt::WidgetsPrivate Qt6::WidgetsPrivate)
    target_link_libraries(${UARTFRAMEPARSER_GUI_NAME} PRIVATE Qt::SerialPort Qt6::SerialPort Qt::SerialPortPrivate Qt6::SerialPortPrivate)
    target_link_libraries(${UARTFRAMEPARSER_GUI_NAME} PRIVATE ${LIBUARTFRAMEPARSER_NAME})

    if (APPLE)
    #set_target_properties(${UARTFRAMEPARSER_GUI_NAME} PROPERTIES
    #    MACOSX_BUNDLE_GUI_IDENTIFIER "quartframeparser.zenghome.cn"
    #    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    #    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    #)
    endif()

    if(QT_VERSION_MAJOR EQUAL 6)
        qt_finalize_executable(${UARTFRAMEPARSER_GUI_NAME})
    endif()

    if (WIN32)
        if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
            file(GLOB QT_QPA_PLATFORM_PLUGIN_PATH ${QT_DIR}/../../debug/Qt6/plugins)
        else()
            file(GLOB QT_QPA_PLATFORM_PLUGIN_PATH ${QT_DIR}/../../Qt6/plugins)
        endif()
        string(JOIN "-" TARBALL_NAME ${CMAKE_PROJECT_NAME} ${GIT_BRANCH} ${GIT_REV} ${CMAKE_BUILD_TYPE})
        string(TOLOWER ${TARBALL_NAME} TARBALL_NAME)
        add_custom_command(TARGET ${UARTFRAMEPARSER_GUI_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -DNAME=${TARBALL_NAME} -DQT_QPA_PLATFORM_PLUGIN_PATH:PATH=${QT_QPA_PLATFORM_PLUGIN_PATH} -P ${CMAKE_CURRENT_LIST_DIR}/generate_tarball.cmake)
    endif()
endif()

# 添加测试
find_package(GTest)

if (${GTEST_FOUND})

    add_executable(${UARTFRAMEPARSER_TEST_NAME} "test/test.cpp")

    target_include_directories(${UARTFRAMEPARSER_TEST_NAME} PRIVATE ${GTEST_INCLUDE_DIRS})
    target_link_libraries(${UARTFRAMEPARSER_TEST_NAME} PRIVATE GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main ${LIBUARTFRAMEPARSER_NAME})

    enable_testing()
    add_test(NAME ${UARTFRAMEPARSER_TEST_NAME} COMMAND ${UARTFRAMEPARSER_TEST_NAME})

endif ()
