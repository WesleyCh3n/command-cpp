cmake_minimum_required(VERSION 3.10)

project(command-win)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

option(BUILD_EXAMPLES "Build examples" ON)

include(cmake/CPM.cmake)
CPMAddPackage("gh:fmtlib/fmt#10.1.1")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if(BUILD_EXAMPLES)
add_executable(basic examples/basic.cc)
target_link_libraries(basic fmt::fmt)
endif()
