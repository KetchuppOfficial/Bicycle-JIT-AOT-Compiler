add_executable(ilist_tests
    src/ilist.cpp
)

target_link_libraries(ilist_tests
PRIVATE
    GTest::GTest
    GTest::gtest_main
    Threads::Threads
    bjac::ilist
)

gtest_discover_tests(ilist_tests)
