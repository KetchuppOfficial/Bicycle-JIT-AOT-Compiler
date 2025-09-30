cmake_minimum_required(VERSION 3.20)

project(bicycle-jit-aot-compiler LANGUAGES CXX)

if (${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
    message(FATAL_ERROR "In-source build is forbidden")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(bjac-ir
    src/IR/basic_block.cpp
    src/IR/function.cpp
    src/IR/instruction.cpp
)
target_include_directories(bjac-ir PUBLIC include)
target_compile_options(bjac-ir
PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:/Wall>
    $<$<CXX_COMPILER_ID:GNU,Clang>: -Wall -Wextra -pedantic>
)

add_executable(fibonacci-test src/main.cpp)
target_link_libraries(fibonacci-test PRIVATE bjac-ir)
