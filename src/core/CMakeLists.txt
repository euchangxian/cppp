add_library(core_lib INTERFACE)

target_link_libraries(core_lib
  INTERFACE
  Boost::boost
)

target_include_directories(core_lib
  INTERFACE
  ${CMAKE_SOURCE_DIR}
)
