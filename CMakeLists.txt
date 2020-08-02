cmake_minimum_required(VERSION 2.8.12)

project(twitse)

option(VCTCXO "DAC controlled VCTCXO" OFF)

if (VCTCXO)
    message(STATUS "DAC/VCTCXO enabled")
    add_definitions(-DVCTCXO)
endif()

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -std=c++11")
# silence the Qt deprecation warnings that arrived somewhere around 5.15
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
# These C flags are intended to match a Raspberry Pi 3. For other versions they should be adjusted accordingly.
# See "GCC compiler optimization for ARM-based systems" https://gist.github.com/fm4dd/c663217935dc17f0fc73c9c81b0aa845
# -Wno-psabi silence gcc 7.1 on rpi
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi -march=armv8-a+crc -mfloat-abi=hard -mfpu=neon-fp-armv8 -mneon-for-64bits -mtune=cortex-a53")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftree-vectorize -ffast-math -funsafe-math-optimizations -pipe -O3 -g0")
endif()

set(CMAKE_AUTOMOC ON)

set(TWITSEROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(ignoreMe ${CMAKE_PREFIX_PATH})
set(ignoreMe ${QT_QMAKE_EXECUTABLE})

add_custom_target(README SOURCES README.md doc/software.md doc/RPI.md doc/VCTCXO.md .travis.yml)

add_subdirectory(control)
add_subdirectory(util)
add_subdirectory(network)
add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(dataanalysis)
