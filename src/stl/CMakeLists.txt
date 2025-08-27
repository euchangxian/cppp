add_library(stl_lib INTERFACE)

target_include_directories(stl_lib
  INTERFACE
  ${CMAKE_SOURCE_DIR}
)

add_subdirectory(tests)
