cmake_minimum_required(VERSION 3.17)
project(os_ex2)

set(CMAKE_CXX_STANDARD 14)

add_executable(os_ex2 demo_jmp.c uthreads.h thread.cpp thread.h)