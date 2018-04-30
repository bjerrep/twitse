cmake_minimum_required(VERSION 2.8.12)

project(twitse)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -std=c++11")

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
# -Wno-psabi silence gcc 7.1 on rpi
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi -mcpu=cortex-a53 -mfloat-abi=hard -mfpu=neon-fp-armv8 -mneon-for-64bits")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftree-vectorize -ffast-math -funsafe-math-optimizations -pipe -O3 -g3")
endif()

set(CMAKE_AUTOMOC ON)

add_custom_target(README SOURCES README.md doc/TLDR.md doc/RPI.md)

add_subdirectory(control)
add_subdirectory(util)
add_subdirectory(network)
add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(dataanalysis)
