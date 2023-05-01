cmake_minimum_required(VERSION 3.16)
project(OS_ex2)

set(CMAKE_CXX_STANDARD  11)

include_directories(.)

add_executable(
        uthreads.h
        uthreads.cpp
        utils.h
        utils.cpp
        tests/test6_no_out.cpp
        )
