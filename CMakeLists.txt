cmake_minimum_required (VERSION 3.15)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Wpedantic -g")
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

project (chip8)

add_executable(chip8 main.cpp)

find_package(SFML COMPONENTS graphics window system)
find_package(fmt)
target_link_libraries (chip8 sfml-graphics fmt)
set_property(TARGET chip8 PROPERTY CXX_STANDARD 17)
