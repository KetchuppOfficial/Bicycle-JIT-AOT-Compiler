cmake_minimum_required(VERSION 3.20)

project(bicycle-jit-aot-compiler LANGUAGES CXX)

if (${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
    message(FATAL_ERROR "In-source build is forbidden")
endif()

if (NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

option(BJAC_BUILD_TESTS "build tests")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/defaults.cmake)

add_library(ilist INTERFACE)
add_library(bjac::ilist ALIAS ilist)
target_link_libraries(ilist INTERFACE bjac::defaults)
target_sources(ilist INTERFACE
FILE_SET
    headers
TYPE
    HEADERS
BASE_DIRS
    include
FILES
    bjac/utils/ilist_iterator.hpp
    bjac/utils/ilist_node.hpp
    bjac/utils/ilist.hpp
)

add_library(ir
    src/IR/basic_block.cpp
    src/IR/function.cpp
    src/IR/instruction.cpp
)
add_library(bjac::ir ALIAS ir)
target_include_directories(ir PUBLIC include)
target_link_libraries(ir
PUBLIC
    bjac::ilist
    bjac::defaults
)

add_executable(fibonacci-test src/main.cpp)
target_link_libraries(fibonacci-test PRIVATE bjac::ir)

if (BJAC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
