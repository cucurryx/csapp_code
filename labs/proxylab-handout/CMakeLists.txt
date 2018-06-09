cmake_minimum_required(VERSION 3.9)
project(proxylab_handout)

set(CMAKE_CXX_STANDARD 11)

add_executable(proxylab_handout
        tiny/cgi-bin/adder.c
        tiny/csapp.c
        tiny/csapp.h
        tiny/tiny.c
        csapp.c
        csapp.h
        proxy.c)
