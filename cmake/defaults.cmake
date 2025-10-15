add_library(defaults INTERFACE)
add_library(bjac::defaults ALIAS defaults)
target_compile_features(defaults INTERFACE cxx_std_26)
target_compile_options(defaults
INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/Wall>
    $<$<CXX_COMPILER_ID:GNU,Clang>: -Wall -Wextra -pedantic>
)
