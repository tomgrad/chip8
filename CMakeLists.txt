cmake_minimum_required (VERSION 3.15)
project (hello_sfml)


add_executable(main main.cpp)

find_package(SFML COMPONENTS graphics window system)
target_link_libraries (main sfml-graphics)
