cmake_minimum_required(VERSION 3.16)
project(os_ex2)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(os_ex2
        ex_2_tests_updated/uthreads.cpp
        ex_2_tests_updated/uthreads.h)
