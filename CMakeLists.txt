cmake_minimum_required(VERSION 3.10)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -mtune=native -fPIC")

project(testSkyline VERSION "1.0.0" LANGUAGES CXX)

file(GLOB SOURCES
  src/*.cpp
)
file(GLOB HEADERS
  include/*.h
)

add_executable (${PROJECT_NAME} ${SOURCES} ${HEADERS})




