include(GoogleTest)

set(TEST_SRCS
  Vector.t.cpp
  UniquePointer.t.cpp
)

add_executable(stl_tests
  ${TEST_SRCS}
)

target_link_libraries(stl_tests
  PRIVATE
  GTest::gtest_main
  stl_lib
)

gtest_discover_tests(stl_tests)
