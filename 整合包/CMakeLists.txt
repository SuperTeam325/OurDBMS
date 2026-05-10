cmake_minimum_required(VERSION 3.16)

project(DBMS VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# AUTOUIC 去根目录找 .ui 文件
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Qt6 REQUIRED COMPONENTS Widgets)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/DCL)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/DQL)

add_compile_definitions(PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")

set(PROJECT_SOURCES
    main.cpp
    mainwindow.h
    mainwindow.cpp

    dialog.h
    dialog.cpp

    DDL.h
    DDL.cpp
    DML.h
    DML.cpp

    Lexer.h
    Lexer.cpp
    Parser.h
    Parser.cpp

    Log.h
    Log.cpp
    Index.h
    Index.cpp

    AST.h

    DQL/DQL.h
    DQL/DQL.cpp

    # DCL
    DCL/dcl_types.h
    DCL/auth_service.h
    DCL/auth_service.cpp
    DCL/dcl_facade.h
    DCL/dcl_facade.cpp
    DCL/logindialog.h
    DCL/logindialog.cpp
    DCL/permission_service.h
    DCL/permission_service.cpp
    DCL/session_manager.h
    DCL/session_manager.cpp
    DCL/system_db_bootstrap.h
    DCL/system_db_bootstrap.cpp
    DCL/user_repository.h
    DCL/user_repository.cpp
)

# UI 文件在项目根目录
set(PROJECT_UIS
    mainwindow.ui
    dialog.ui
    logindialog.ui
)

qt_add_executable(DBMS
    ${PROJECT_SOURCES}
    ${PROJECT_UIS}
)

target_link_libraries(DBMS PRIVATE Qt6::Widgets)

set_target_properties(DBMS PROPERTIES WIN32_EXECUTABLE TRUE)
