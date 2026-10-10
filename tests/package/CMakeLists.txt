# Consumer of the installed package, built by CI after `cmake --install`:
# checks that find_package, the exported target, the headers and the system
# libraries work for a project outside this repository.
cmake_minimum_required(VERSION 3.21)
project(inpututil_package_test LANGUAGES CXX)

find_package(inpututil 2 REQUIRED)

add_executable(package_test main.cpp)
target_link_libraries(package_test PRIVATE inpututil::inpututil)
