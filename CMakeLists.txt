cmake_minimum_required(VERSION 3.0)

project(mlc)

add_executable(mlc main.cpp)

install(TARGETS mlc RUNTIME DESTINATION bin)
