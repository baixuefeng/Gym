﻿set(SQLITE_SOURCE_CODE 
    ${CMAKE_CURRENT_LIST_DIR}/sqlite3.c
    ${CMAKE_CURRENT_LIST_DIR}/sqlite3.h
    ${CMAKE_CURRENT_LIST_DIR}/SQLiteCpp/src/Backup.cpp
    ${CMAKE_CURRENT_LIST_DIR}/SQLiteCpp/src/Column.cpp
    ${CMAKE_CURRENT_LIST_DIR}/SQLiteCpp/src/Database.cpp
    ${CMAKE_CURRENT_LIST_DIR}/SQLiteCpp/src/Exception.cpp
    ${CMAKE_CURRENT_LIST_DIR}/SQLiteCpp/src/Statement.cpp
    ${CMAKE_CURRENT_LIST_DIR}/SQLiteCpp/src/Transaction.cpp
    ${CMAKE_CURRENT_LIST_DIR}/SQLiteCpp/Assertion.h
    ${CMAKE_CURRENT_LIST_DIR}/SQLiteCpp/Backup.h
    ${CMAKE_CURRENT_LIST_DIR}/SQLiteCpp/Column.h
    ${CMAKE_CURRENT_LIST_DIR}/SQLiteCpp/Database.h
    ${CMAKE_CURRENT_LIST_DIR}/SQLiteCpp/Exception.h
    ${CMAKE_CURRENT_LIST_DIR}/SQLiteCpp/SQLiteCpp.h
    ${CMAKE_CURRENT_LIST_DIR}/SQLiteCpp/Statement.h
    ${CMAKE_CURRENT_LIST_DIR}/SQLiteCpp/Transaction.h
    ${CMAKE_CURRENT_LIST_DIR}/SQLiteCpp/Utils.h
    ${CMAKE_CURRENT_LIST_DIR}/SQLiteCpp/VariadicBind.h
    )

add_compile_definitions(SQLITE_ENABLE_COLUMN_METADATA)
include_directories("${CMAKE_CURRENT_LIST_DIR}")
