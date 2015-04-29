cmake_minimum_required(VERSION 3.1)
project(bait)

file(GLOB_RECURSE HPPS bait/*.hpp)
add_library(bait INTERFACE)
set_property(TARGET bait PROPERTY INTERFACE_SOURCES ${HPPS})
target_include_directories(bait INTERFACE bait)
set_property(TARGET bait PROPERTY CXX_STANDARD 14)

set(SOURCE_FILES main.cpp)
add_executable(bait_test EXCLUDE_FROM_ALL ${SOURCE_FILES})
set_property(TARGET bait_test PROPERTY CXX_STANDARD 14)
