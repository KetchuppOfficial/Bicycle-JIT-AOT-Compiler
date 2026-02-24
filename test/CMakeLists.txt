find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

include(GoogleTest)

add_library(tests_common INTERFACE)
target_sources(tests_common INTERFACE
    FILE_SET HEADERS
    BASE_DIRS include
    FILES include/test/common.hpp
)
target_link_libraries(tests_common
INTERFACE
    bjac::ir
    GTest::gtest_main
    Threads::Threads
)

add_subdirectory(analysis)
add_subdirectory(graphs)
add_subdirectory(IR)
add_subdirectory(transforms)
add_subdirectory(utils)
