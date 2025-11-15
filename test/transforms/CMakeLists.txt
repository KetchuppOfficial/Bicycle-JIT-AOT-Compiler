add_executable(transforms_tests
    src/constant_folding.cpp
)

target_link_libraries(transforms_tests
PRIVATE
    GTest::gtest_main
    Threads::Threads
    bjac::transforms
)

gtest_discover_tests(transforms_tests)
