add_executable(bjac_graphs_tests
    src/dominator_tree.cpp
    src/linear_order.cpp
    src/loop_tree.cpp
)

target_link_libraries(bjac_graphs_tests
PRIVATE
    tests_common
    bjac::graphs
    bjac::ir
)

gtest_discover_tests(bjac_graphs_tests)
