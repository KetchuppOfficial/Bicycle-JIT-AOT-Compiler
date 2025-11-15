cmake_minimum_required(VERSION 3.30)

project(bicycle-jit-aot-compiler LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

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
    HEADERS
BASE_DIRS
    include
FILES
    include/bjac/utils/ilist_iterator.hpp
    include/bjac/utils/ilist_node.hpp
    include/bjac/utils/ilist.hpp
)

add_library(ir STATIC)
add_library(bjac::ir ALIAS ir)
target_sources(ir PRIVATE
    lib/IR/basic_block.cpp
    lib/IR/function.cpp
    lib/IR/instruction.cpp
)
target_sources(ir PUBLIC
FILE_SET
    HEADERS
BASE_DIRS
    include
FILES
    include/bjac/IR/argument_instruction.hpp
    include/bjac/IR/basic_block.hpp
    include/bjac/IR/binary_operator.hpp
    include/bjac/IR/branch_instruction.hpp
    include/bjac/IR/constant_instruction.hpp
    include/bjac/IR/function.hpp
    include/bjac/IR/icmp_instruction.hpp
    include/bjac/IR/instruction.hpp
    include/bjac/IR/phi_instruction.hpp
    include/bjac/IR/ret_instruction.hpp
    include/bjac/IR/type.hpp
    include/bjac/IR/value.hpp
)
target_link_libraries(ir
PUBLIC
    bjac::ilist
    bjac::defaults
)

add_library(graphs INTERFACE)
add_library(bjac::graphs ALIAS graphs)
target_link_libraries(graphs INTERFACE bjac::defaults)
target_sources(graphs INTERFACE
FILE_SET
    HEADERS
BASE_DIRS
    include
FILES
    include/bjac/graphs/dfs.hpp
    include/bjac/graphs/dominator_tree.hpp
    include/bjac/graphs/graph_traits.hpp
    include/bjac/graphs/loop_tree.hpp
    include/bjac/graphs/loop.hpp
)

add_library(transforms STATIC)
add_library(bjac::transforms ALIAS transforms)
target_link_libraries(transforms
PRIVATE
    bjac::graphs
PUBLIC
    bjac::defaults
    bjac::ir
)
target_sources(transforms PRIVATE
    lib/transforms/constant_folding.cpp
)
target_sources(transforms PUBLIC
FILE_SET
    HEADERS
BASE_DIRS
    include
FILES
    include/bjac/transforms/constant_folding.hpp
)

if (BJAC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

install(TARGETS ir ilist graphs transforms
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT BJAC_Runtime
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT BJAC_Runtime
            NAMELINK_COMPONENT BJAC_Development
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT BJAC_Development
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
                     COMPONENT BJAC_Development
)
