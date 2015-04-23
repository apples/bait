cmake_minimum_required(VERSION 3.2)
project(BehaviorTreeProj)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall -Wextra -pedantic")

set(SOURCE_FILES main.cpp)
add_executable(BehaviorTree ${SOURCE_FILES})
