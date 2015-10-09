cmake_minimum_required(VERSION 2.8.8)

project(splice-pool)

add_definitions(${CMAKE_CXX_FLAGS} "-std=c++11")
add_definitions(${CMAKE_CXX_FLAGS} "-Wall")
add_definitions(${CMAKE_CXX_FLAGS} "-pedantic")
add_definitions(${CMAKE_CXX_FLAGS} "-fexceptions")
add_definitions(${CMAKE_CXX_FLAGS} "-fPIC")

install(FILES splice-pool.hpp DESTINATION include/splice-pool)

add_subdirectory(third/gtest-1.7.0)
include_directories(. third/gtest-1.7.0/include third/gtest-1.7.0)
add_subdirectory(test)

