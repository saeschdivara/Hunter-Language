cmake_minimum_required(VERSION 3.19)
project(Hunter_Language)

set(CMAKE_CXX_STANDARD 20)

set(CONAN_DISABLE_CHECK_COMPILER 1)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)  # Includes the contents of the conanbuildinfo.cmake file.
conan_basic_setup()  # Prepares the CMakeList.txt for Conan.

add_subdirectory(Compiler)
