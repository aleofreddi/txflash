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

# Enable verbose makefile
set(CMAKE_VERBOSE_MAKEFILE TRUE)

include_directories(
        ../include
)

add_executable(
        unit_test

        # Tested
        ../include/tx_flash.hh
        ../include/tx_flash_stm32f4.hh
        ../include/tx_flash_stm32f7.hh

        # Tested
        main.cc
        tx_flash_test.cc
)

enable_testing()
add_test(NAME unit_test COMMAND unit_test)

