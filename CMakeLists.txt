cmake_minimum_required(VERSION 4.1)

set(CMAKE_C_COMPILER "gcc-15")
set(CMAKE_CXX_COMPILER "g++-15")

project(cppp VERSION 0.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic"
)

add_compile_options(-fno-modules)
add_link_options(-fno-modules)

if(CMAKE_BUILD_TYPE STREQUAL "DEBUG")
  add_compile_options(-fsanitize=address -fsanitize=undefined)
  add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

find_package(Boost 1.89.0 REQUIRED)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_subdirectory(src)
