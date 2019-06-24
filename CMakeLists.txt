#
# TxFlash cmake list file
#
# @author Andrea Leofreddi <a.leofreddi@quantica.io>
#
cmake_minimum_required(VERSION 2.8.12)
project(TxFlash CXX)

# Conan
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# Enforce C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Produce verbose makefile
set(CMAKE_VERBOSE_MAKEFILE ON)

enable_testing()
add_subdirectory(test_package)
