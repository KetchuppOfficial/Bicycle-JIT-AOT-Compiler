add_executable(bjac_graphs_tests
    src/dominator_tree.cpp
    src/loop_tree.cpp
)

target_link_libraries(bjac_graphs_tests
PRIVATE
    GTest::GTest
    GTest::gtest_main
    Threads::Threads
    bjac::graphs
    bjac::ir
)

gtest_discover_tests(bjac_graphs_tests)
