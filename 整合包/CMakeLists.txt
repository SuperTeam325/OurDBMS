cmake_minimum_required(VERSION 3.16)

project(DBMS VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)


include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/DCL)


add_compile_definitions(PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    DDL.h
    DDL.cpp
    Lexer.h
    Lexer.cpp
    Parser.h
    Parser.cpp

    dialog.h
    dialog.cpp
    dialog.ui

    # DCL 真实路径！！！
    DCL/auth_service.h
    DCL/auth_service.cpp
    DCL/dcl_facade.h
    DCL/dcl_facade.cpp
    DCL/dcl_types.h
    DCL/logindialog.h
    DCL/logindialog.cpp
    DCL/logindialog.ui
    DCL/permission_service.h
    DCL/permission_service.cpp
    DCL/session_manager.h
    DCL/session_manager.cpp
    DCL/system_db_bootstrap.h
    DCL/system_db_bootstrap.cpp
    DCL/test.cpp
    DCL/user_repository.h
    DCL/user_repository.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(DBMS
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        DML.h
        DML.cpp
        Log.h
        Log.cpp
        DQL/DQL.h
        DQL/DQL.cpp
    )
else()
    if(ANDROID)
        add_library(DBMS SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(DBMS
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(DBMS PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOS_BUNDLE_IDENTIFIER com.example.DBMS)
endif()

set_target_properties(DBMS PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOS_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOS_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOS_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS DBMS
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(DBMS)
endif()
