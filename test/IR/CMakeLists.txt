add_executable(fibonacci_test src/fibonacci.cpp)
target_link_libraries(fibonacci_test
PRIVATE
    bjac::ir
    bjac::defaults
    tests_common
)

add_executable(bjac_ir_tests
    src/function.cpp
)
target_link_libraries(bjac_ir_tests
PRIVATE
    GTest::gtest_main
    Threads::Threads
    bjac::ir
    tests_common
)

gtest_discover_tests(bjac_ir_tests)
