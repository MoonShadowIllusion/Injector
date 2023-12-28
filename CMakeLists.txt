cmake_minimum_required(VERSION 3.5.0)
project(Injector VERSION 0.1.0 LANGUAGES C CXX)

add_executable(Injector
        src/inject.cpp
        src/main.cpp
        src/uac.rc
)