add_library(bjac_defaults INTERFACE)
add_library(bjac::defaults ALIAS bjac_defaults)
target_compile_features(bjac_defaults INTERFACE cxx_std_26)
target_compile_options(bjac_defaults
INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/Wall>
    $<$<CXX_COMPILER_ID:GNU,Clang>: -Wall -Wextra -pedantic>
)
