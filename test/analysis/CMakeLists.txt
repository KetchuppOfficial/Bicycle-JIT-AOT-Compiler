add_executable(bjac_analysis_tests
    src/lifetime.cpp
    src/liveness.cpp
)

target_link_libraries(bjac_analysis_tests
PRIVATE
    tests_common
    bjac::analysis
)

gtest_discover_tests(bjac_analysis_tests)
