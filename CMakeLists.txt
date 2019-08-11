cmake_minimum_required(VERSION 3.0)

project(openvrvisualizer LANGUAGES C)

add_executable(openvrvisualizer main.c)

install(TARGETS openvrvisualizer RUNTIME DESTINATION bin)
