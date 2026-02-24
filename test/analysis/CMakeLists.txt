add_executable(bjac_analysis_tests
    src/lifetime.cpp
)

target_link_libraries(bjac_analysis_tests
PRIVATE
    GTest::GTest
    GTest::gtest_main
    Threads::Threads
    bjac::analysis
)

gtest_discover_tests(bjac_analysis_tests)
