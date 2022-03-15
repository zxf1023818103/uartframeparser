cmake_minimum_required(VERSION 3.8)

include(FetchContent)
FetchContent_Declare(vcpkg
    vcpkg
    URL "https://github.com/microsoft/vcpkg/archive/master.zip")
FetchContent_MakeAvailable(vcpkg)
<<<<<<< HEAD

project("uartframeparser")

set(CMAKE_TOOLCHAIN_FILE ${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake
  CACHE STRING "Vcpkg toolchain file")
=======
set(CMAKE_TOOLCHAIN_FILE ${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake)

project("uartframeparser")
>>>>>>> 4b5e2a9c2724c29e89ddd5e8aba4a21d4e5116f9

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
add_library(${LIBUARTFRAMEPARSER_NAME} STATIC "lib/config_parse.c" "lib/frame_parse.c" "include/uartframeparser.h" "lib/buffer.c" "lib/expression.c" "lib/frame_read.c" "lib/frame_decode.c")

set(_IMPORT_PREFIX "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
find_package(cJSON REQUIRED)
target_link_libraries(${LIBUARTFRAMEPARSER_NAME} PRIVATE cjson)

find_package(Lua REQUIRED)
target_link_libraries(${LIBUARTFRAMEPARSER_NAME} PRIVATE ${LUA_LIBRARIES})
target_include_directories(${LIBUARTFRAMEPARSER_NAME} PRIVATE ${LUA_INCLUDE_DIR})

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets LinguistTools SerialPort)
if (QT_FOUND)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets LinguistTools SerialPort REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets LinguistTools SerialPort REQUIRED)

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
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET uartframeparser APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html·#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
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

target_link_libraries(${UARTFRAMEPARSER_GUI_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::SerialPort ${LIBUARTFRAMEPARSER_NAME})

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
