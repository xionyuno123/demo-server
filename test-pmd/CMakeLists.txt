cmake_minimum_required(VERSION 3.2)

project(cmake-project-template)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -O3")

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(src)
# add_subdirectory(test)


