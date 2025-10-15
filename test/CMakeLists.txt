find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

include(GoogleTest)

add_subdirectory(graphs)
add_subdirectory(IR)
add_subdirectory(utils)
