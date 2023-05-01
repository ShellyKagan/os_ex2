cmake_minimum_required(VERSION 3.16)
project(OS_ex2)

set(CMAKE_CXX_STANDARD  11)

include_directories(.)

add_executable(
        uthreads.h
        uthreads.cpp
        tests/jona1.cpp
        )
