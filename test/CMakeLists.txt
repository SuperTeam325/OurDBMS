cmake_minimum_required(VERSION 3.16)

# Stand-alone build of the DBMS test tool.
# Include this from the parent CMakeLists.txt with add_subdirectory(test),
# or build it separately by pointing cmake at this directory.

project(DBMS_Test LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# Re-use DDL and DCL sources from the parent directory
set(DDL_SOURCES
    ../DDL/DDL.cpp
    ../DDL/DDL.h
    ../DDL/Lexer.cpp
    ../DDL/Lexer.h
    ../DDL/Parser.cpp
    ../DDL/Parser.h
    ../DDL/mainwindow.cpp
    ../DDL/mainwindow.h
    ../DDL/mainwindow.ui
    ../DDL/dialog.cpp
    ../DDL/dialog.h
    ../DDL/dialog.ui
)

file(GLOB DCL_SOURCES CONFIGURE_DEPENDS
    ../DCL/*.cpp
    ../DCL/*.h
)

# Test tool sources
set(TEST_SOURCES
    main.cpp
    test_runner.cpp
    test_runner.h
)

set(PROJECT_SOURCES
    ${TEST_SOURCES}
    ${DDL_SOURCES}
    ${DCL_SOURCES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(DBMS_Test
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(DBMS_Test
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(DBMS_Test PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Inject the project's dataDB absolute path at compile time.
# DBMS_Test.exe is one directory deeper than DBMS.exe, so runtime
# relative-path resolution (../../dataDB) would resolve to a different
# location. This compile definition guarantees the test tool always
# uses the same dataDB/ as the main project.
target_compile_definitions(DBMS_Test PRIVATE
    PROJECT_DATA_DB_PATH="${CMAKE_SOURCE_DIR}/dataDB"
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(DBMS_Test)
endif()
