#
# TxFlash/test cmake list file
#
# @author Andrea Leofreddi <a.leofreddi@quantica.io>
#
cmake_minimum_required(VERSION 2.8.12)
project(TxFlashTest CXX)

# Conan
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# Enforce C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable coverage for gcc and clang
if((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
endif()

include_directories(
        ../include
)

add_executable(
        unit_test

        # Tested
        ../include/txflash.hh
        ../include/txflash_stm32f4.hh
        ../include/txflash_stm32f7.hh

        # Tested
        main.cc
        txflash_test.cc
)

enable_testing()
add_test(NAME unit_test COMMAND unit_test)

