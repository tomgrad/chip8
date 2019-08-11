cmake_minimum_required (VERSION 3.15)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Wpedantic")
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

project (hello_sfml)


add_executable(main main.cpp)

find_package(SFML COMPONENTS graphics window system)
find_package(fmt)
target_link_libraries (main sfml-graphics fmt)
