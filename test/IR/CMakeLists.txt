add_executable(fibonacci_test src/fibonacci.cpp)
target_link_libraries(fibonacci_test
PRIVATE
    bjac::ir
    bjac::defaults
)
