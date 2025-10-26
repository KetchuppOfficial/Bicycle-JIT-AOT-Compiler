add_executable(graphs_tests
    src/dominator_tree.cpp
    src/loop_tree.cpp
)

target_link_libraries(graphs_tests
PRIVATE
    GTest::GTest
    GTest::gtest_main
    Threads::Threads
    bjac::graphs
    bjac::ir
)

gtest_discover_tests(graphs_tests)
