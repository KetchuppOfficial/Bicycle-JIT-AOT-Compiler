add_executable(bjac_transforms_tests
    src/constant_folding.cpp
)

target_link_libraries(bjac_transforms_tests
PRIVATE
    GTest::gtest_main
    Threads::Threads
    bjac::transforms
)

gtest_discover_tests(bjac_transforms_tests)
