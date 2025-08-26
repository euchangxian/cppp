add_library(stl_lib INTERFACE)

target_include_directories(stl_lib INTERFACE .)

add_subdirectory(tests)
